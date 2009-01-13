# Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

require 'RRD'
require 'fileutils'
require 'time'

module Milter
  class Session
    attr_accessor :name, :start_time, :end_time
    def initialize(name)
      @name = name
      @start_time = nil
      @end_time = nil
    end
  end

  class Mail
    attr_accessor :status, :time, :key
    def initialize(status, time)
      @key = status
      @status = status
      @time = time
    end
  end

  class Stop
    attr_accessor :state, :name, :time
    def initialize(state, name, time)
      @state = state
      @name = name
      @time = time
    end

    def key
      @state
    end
  end

  class RRD
    class TimeRange
      def initialize(rrd_step, rows)
        @rrd_step = rrd_step
        @rows = rows
      end
    end

    class DayRange < TimeRange
      def name
        "Day"
      end

      def steps
        (3600 * 24) / (@rrd_step * @rows)
      end
    end

    class WeekRange < TimeRange
      def name
        "Week"
      end

      def steps
        DayRange.new(@rrd_step, @rows).steps * 7
      end
    end

    class MonthRange < TimeRange
      def name
        "Month"
      end

      def steps
        WeekRange.new(@rrd_step, @rows).steps * 5
      end
    end

    class YearRange < TimeRange
      def name
        "Year"
      end

      def steps
        MonthRange.new(@rrd_step, @rows).steps * 12
      end
    end

    class TimeSpan
      attr_accessor :name
      def initialize(name)
        @name = name
      end

      def step
        case @name
        when "second"
          1
        when "minute"
          60
        when "hour"
          3600
        end
      end

      def short_name
        case @name
        when "second"
          "sec"
        when "minute"
          "min"
        when "hour"
          "hour"
        end
      end

      def rows
        case @name
        when "second"
          86400 # 1day
        when "minute"
          43200 # 4weeks
        when "hour"
          8760 # 365days
        end
      end

      def default_start_time
        case @name
        when "second"
          "-3h" # 1hour
        when "minute"
          "-24h" # 12hours
        when "hour"
          "-7d" # 1day
        end
      end

      def adjust_time(time)
        case @name
        when "second"
          time
        when "minute"
          Time.utc(0, time.min, time.hour, time.mday, time.mon, time.year, time.wday, time.yday, time.isdst, time.zone)
        when "hour"
          Time.utc(0, 0, time.hour, time.mday, time.mon, time.year, time.wday, time.yday, time.isdst, time.zone)
        end
      end
    end

    class Count < Hash
      def [](key)
        super(key) ? super(key) : store(key, Hash.new(0))
      end
    end

    class Data
      def initialize(countings)
        @countings = countings
      end

      def empty?
        return true unless @countings
        @countings.each_value do |counting|
          return false unless counting.empty?
        end
        true
      end

      def last_time
        last_time = 0
        @countings.each_value do |counting|
          next if counting.empty?
          last_one = counting.sort { |a, b| a[0] <=> b[0] }.last[0]
          last_time = last_time > last_one ? last_time : last_one
        end
        last_time
      end

      def first_time
        first_time = 0
        @countings.each_value do |counting|
          next if counting.empty?
          first_one = counting.sort { |a, b| a[0] <=> b[0] }.first[0]
          first_time = first_time > first_one ? first_time : first_one
        end
        first_time
      end

      def [](key)
        @countings[key]
      end

      def []=(key, value)
        @countings[key] = value
      end
    end

    class Graph
      def initialize(rrd_directory, update_time)
        @rrd_directory = rrd_directory
        @update_time = update_time
        @data = []
        @items = nil
        @title = nil
        @vertical_label = nil
        @rrd_step = 60
        @width = 600
        @points_per_sample = 3
      end

      def rows
        @width / @points_per_sample
      end

      def count(time_span, last_update_time)
        counting = Count.new()
        @data.each do |datum|
          time = time_span.adjust_time(datum.time)

          # ignore sessions which has been already registerd due to RRD.update fails on past time stamp
          if last_update_time
            next if time <= last_update_time
          end

          # ignore recent sessions due to RRD.update fails on past time stamp
          next if time >= time_span.adjust_time(@update_time)

          time = time.to_i
          key = datum.key

          count = counting[key][time]
          count = 0 if count == "U"
          count += 1
          counting[key][time] = count
        end
        counting
      end

      def collect_data(time_span, last_update_time)
        counting = count(time_span, last_update_time)
        Data.new(counting)
      end

      def update_db(time_span)
        last_update_time = ::RRD.last(rrd_file) if File.exist?(rrd_file)
        if last_update_time and last_update_time <= Time.at(0)
          last_update_time = Time.at(Integer(`rrdtool last '#{rrd_file}'`))
        end

        data = collect_data(time_span, last_update_time)
        return if data.empty?

        end_time = data.last_time
        if last_update_time
          start_time = last_update_time + time_span.step
        else
          start_time = data.first_time
        end

        create_rrd(time_span, start_time, *@items) unless File.exist?(rrd_file)

        start_time.to_i.step(end_time, time_span.step) do |time|
          counts = []
          @items.each do |item|
            counts << data[item][time]
          end

          next if counts.uniq.length == 1 and counts.uniq.include?("U")

          # puts("update #{rrd} with #{Time.at(time)}  #{counts.join(':')}")
          ::RRD.update(rrd_file,
                       "#{time}:#{counts.join(':')}")
        end
      end

      def update
        update_db(TimeSpan.new("minute"))
      end

      def create_rrd(time_span, start_time, *args)
        step = @rrd_step

        day_range = DayRange.new(@rrd_step, rows)
        week_range = WeekRange.new(@rrd_step, rows)
        month_range = MonthRange.new(@rrd_step, rows)
        year_range = YearRange.new(@rrd_step, rows)
        ::RRD.create(rrd_file,
                     "--start", (start_time - 1).to_i.to_s,
                     "--step", step,
                     "RRA:MAX:0.5:#{day_range.steps}:#{rows}",
                     "RRA:MAX:0.5:#{week_range.steps}:#{rows}",
                     "RRA:MAX:0.5:#{month_range.steps}:#{rows}",
                     "RRA:MAX:0.5:#{year_range.steps}:#{rows}",
                     "RRA:AVERAGE:0.5:#{day_range.steps}:#{rows}",
                     "RRA:AVERAGE:0.5:#{week_range.steps}:#{rows}",
                     "RRA:AVERAGE:0.5:#{month_range.steps}:#{rows}",
                     "RRA:AVERAGE:0.5:#{year_range.steps}:#{rows}",
                     *args.map{|arg| "DS:#{arg}:GAUGE:#{step * 3}:0:U"})
      end

      def output_graph(time_span, options={}, *args)
        start_time = options[:start_time] || time_span.default_start_time
        end_time = options[:end_time] || guess_end_time(start_time)
        graph_tag = options[:graph_tag] || time_span.name
        width = options[:width] || @width
        height = options[:height] || 200

        return nil unless File.exist?(rrd_file)
        last_update_time = ::RRD.last(rrd_file)
        if last_update_time <= Time.at(0)
          last_update_time = Time.at(Integer(`rrdtool last '#{rrd_file}'`))
        end

        time_stamp = last_update_time.strftime("%a %b %d %H:%M:%S %Z %Y")
        title = "#{@title} - #{time_stamp}"
        vertical_label = "#{@vertical_label}/#{time_span.short_name}"
        name = graph_name(graph_tag)

        items = @items.inject([]) do |_items, item|
          _items + ["DEF:#{item}=#{rrd_file}:#{item}:AVERAGE",
                    "DEF:max_#{item}=#{rrd_file}:#{item}:MAX",
                    "CDEF:n_#{item}=#{item},UN,0,#{item},IF",
                    "CDEF:real_#{item}=#{item},#{@rrd_step},*",
                    "CDEF:real_max_#{item}=max_#{item},#{@rrd_step},*",
                    "CDEF:real_n_#{item}=n_#{item},#{@rrd_step},*",
                    "CDEF:total_#{item}=PREV,UN,real_n_#{item},PREV,IF,real_n_#{item},+"]
        end
        ::RRD.graph(name,
                    "--title", title,
                    "--vertical-label", vertical_label,
                    "--start", start_time.to_s,
                    "--end", end_time.to_s,
                    "--width", width.to_s,
                    "--height", height.to_s,
                    "--alt-y-grid",
                    *(items + args))
        name
      end

      private
      def build_path(*paths)
        File.join(*([@rrd_directory, *paths].compact))
      end

      def guess_end_time(start_time)
        if start_time.is_a?(String)
          "now"
        else
          end_time = Time.now.to_i
          step = start_time.abs * @points_per_sample / @width
          end_time - (end_time % step)
        end
      end
    end

    class Session < Graph
      def initialize(rrd_directory, update_time)
        super(rrd_directory, update_time)
        @title = 'milter-manager sessions'
        @vertical_label = "sessions"
        @items = ["smtp", "child"]
        @child_sessions = []
        @client_sessions = []
      end

      def rrd_file
        build_path("milter-log.session.rrd")
      end

      def graph_name(tag)
        build_path("session.#{tag}.png")
      end

      def collect_session(time_stamp, content, sessions, regex)
        if regex.match(content)
          elapsed = $1
          name = $2
          session = Milter::Session.new(name)
          session.end_time = time_stamp
          session.start_time = time_stamp - Float(elapsed)
          sessions << session
        end
        sessions
      end

      def feed(time_stamp, content)
        @child_sessions =
          collect_session(time_stamp, content, @child_sessions,
                          /\A\[milter\]\[end\]\[(.+)\]\(.+\): (.+)\z/)
        @client_sessions =
          collect_session(time_stamp, content, @client_sessions,
                          /\A\[session\]\[end\]\[(.+)\]\(.+\)\z/)
      end

      def count_sessions(sessions, time_span, last_update_time)
        counting = Hash.new(0)
        sessions.each do |session|
          next if session.end_time == nil
          start_time = time_span.adjust_time(session.start_time)
          end_time = time_span.adjust_time(session.end_time)

          # ignore sessions which has been already registerd due to RRD.update fails on past time stamp
          if last_update_time
            next if end_time <= last_update_time
            next if start_time <= last_update_time
          end

          # ignore recent sessions due to RRD.update fails on past time stamp
          next if end_time >= time_span.adjust_time(@update_time)
          next if start_time >= time_span.adjust_time(@update_time)

          start_time = start_time.to_i
          end_time = end_time.to_i

          start_time.step(end_time, time_span.step) do |time|
            time = end_time # FIXME
            count = counting[time]
            count = 0 if count == "U"
            count += 1
            counting[time] = count
            break
          end
        end
        counting
      end

      def collect_data(time_span, last_update_time)
        counting = Count.new()
        counting["smtp"] = count_sessions(@client_sessions, time_span, last_update_time)
        counting["child"] = count_sessions(@child_sessions, time_span, last_update_time)
        Data.new(counting)
      end

      def output_graph(time_span, options={})
        super(time_span, options,
              "AREA:n_smtp#0000ff:SMTP  ",
              "GPRINT:total_smtp:MAX:total\\: %8.0lf sessions",
              "GPRINT:smtp:AVERAGE:avg\\: %6.2lf sessions/min",
              "GPRINT:max_smtp:MAX:max\\: %4.0lf sessions/min\\l",
              "LINE2:n_child#00ff00:milter",
              "GPRINT:total_child:MAX:total\\: %8.0lf sessions",
              "GPRINT:child:AVERAGE:avg\\: %6.2lf sessions/min",
              "GPRINT:max_child:MAX:max\\: %4.0lf sessions/min\\l")
      end
    end

    class MailStatus < Graph
      def initialize(rrd_directory, update_time)
        super(rrd_directory, update_time)
        @vertical_label = "mails"
        @title = 'Processed mails'
        @items = ["pass", "accept",
                  "reject", "discard",
                  "temporary-failure",
                  "quarantine"]
      end

      def rrd_file
        build_path("milter-log.mail.rrd")
      end

      def graph_name(tag)
        build_path("mail.#{tag}.png")
      end

      def feed(time_stamp, content)
        if /\A\[reply\]\[(.+)\]\[(.+)\]\z/ =~ content
          state = $1
          status = $2
          return if status == "continue" and state != "end-of-message"
          status = "pass" if status == "continue"
          @data << Milter::Mail.new(status, time_stamp)
        end
      end

      def output_graph(time_span, options={})
        entries = [
         ["AREA", "pass", "#0000ff", "Pass"],
         ["STACK", "accept", "#00ff00", "Accept"],
         ["STACK", "reject", "#ff0000", "Reject"],
         ["STACK", "discard", "#ffd400", "Discard"],
         ["STACK", "temporary-failure", "#888888", "Temp-Fail"],
         ["STACK", "quarantine", "#a52a2a", "Quarantine"],
        ]
        max_label_size = entries.collect {|_, _, _, label| label.size}.max

        items = []
        entries.each do |type, name, color, label|
          items << "#{type}:#{name}#{color}:#{label.ljust(max_label_size)}"
          items << "GPRINT:total_#{name}:MAX:total\\: %11.0lf mails"
          items << "GPRINT:#{name}:AVERAGE:avg\\: %9.2lf mails/min"
          items << "GPRINT:max_#{name}:MAX:max\\: %7.0lf mails/min\\l"
        end

        path = build_path("milter-log.session.rrd")
        items << "DEF:smtp=#{path}:smtp:AVERAGE"
        items << "DEF:max_smtp=#{path}:smtp:MAX"
        items << "CDEF:n_smtp=smtp,UN,0,smtp,IF"
        items << "CDEF:real_n_smtp=n_smtp,#{@rrd_step},*"
        items << "CDEF:total_smtp=PREV,UN,real_n_smtp,PREV,IF,real_n_smtp,+"

        label = "SMTP".ljust(max_label_size)
        items << "LINE2:n_smtp#000000:#{label}"
        items << "GPRINT:total_smtp:MAX:total\\: %8.0lf sessions"
        items << "GPRINT:smtp:AVERAGE:avg\\: %6.2lf sessions/min"
        items << "GPRINT:max_smtp:MAX:max\\: %4.0lf sessions/min\\l"
        super(time_span, options, *items)
      end
    end

    class Stops < Graph
      def initialize(rrd_directory, update_time)
        super(rrd_directory, update_time)
        @title = 'Stopped milters'
        @vertical_label = "milters"
        @items = ["connect",
                  "helo",
                  "envelope-from",
                  "envelope-recipient",
                  "header",
                  "body",
                  "end-of-message"]
      end

      def rrd_file
        build_path("milter-log.stop.rrd")
      end

      def graph_name(tag)
        build_path("stop.#{tag}.png")
      end

      def feed(time_stamp, content)
        if /\A\[stop\]\[(.+)\]: (.+)\z/ =~ content
          state = $1
          name = $2
          @data << Milter::Stop.new(state, name, time_stamp)
        end
      end

      def collect_data(time_span, last_update_time)
        stop_counting = count(time_span, last_update_time)
        Data.new(stop_counting)
      end

      def output_graph(time_span, options={})
        entries = [
         ["AREA", "connect", "#0000ff"],
         ["STACK", "helo", "#ff00ff"],
         ["STACK", "envelope-from", "#00ffff"],
         ["STACK", "envelope-recipient", "#ffff00"],
         ["STACK", "header", "#a52a2a"],
         ["STACK", "body", "#ff0000"],
         ["STACK", "end-of-message", "#00ff00"],
        ]
        max_name_size = entries.collect {|_, name, _| name.size}.max

        items = []
        entries.each do |type, name, color|
          items << "#{type}:#{name}#{color}:#{name.ljust(max_name_size)}"
          items << "GPRINT:total_#{name}:MAX:total\\: %8.0lf milters"
          items << "GPRINT:#{name}:AVERAGE:avg\\: %6.2lf milters/min"
          items << "GPRINT:max_#{name}:MAX:max\\: %4.0lf milters/min\\l"
        end

        path = build_path("milter-log.session.rrd")
        items << "DEF:child=#{path}:child:AVERAGE"
        items << "DEF:max_child=#{path}:child:MAX"
        items << "CDEF:n_milters=child,UN,0,child,IF"
        items << "CDEF:real_n_milters=n_milters,#{@rrd_step},*"
        items << "CDEF:total_milters=PREV,UN,real_n_milters,PREV,IF,real_n_milters,+"
        items << "CDEF:smaller_n_milters=n_milters,10,/"

        label = 'total'.ljust(max_name_size)
        items << "LINE2:n_milters#000000:#{label}"
        items << "GPRINT:total_milters:MAX:total\\: %8.0lf milters"
        items << "GPRINT:child:AVERAGE:avg\\: %6.2lf milters/min"
        items << "GPRINT:max_child:MAX:max\\: %4.0lf milters/min\\l"
        super(time_span, options, *items)
      end
    end
  end
end

# vi:ts=2:nowrap:ai:expandtab:sw=2
