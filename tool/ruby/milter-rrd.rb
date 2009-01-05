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
    attr_accessor :id, :name, :start_time, :end_time
    def initialize(id, name)
      @id = id
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

  class PassChild
    attr_accessor :name, :state, :time, :key
    def initialize(name, state, time)
      @name = name
      @state = state
      @time = time
      @key = state
    end
  end

  class RRD
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
        super(key) ? super(key) : store(key, Hash.new("U"))
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
      def initialize(rrd_directory, log, update_time)
        @rrd_directory = rrd_directory
        @log = log
        @update_time = update_time
        @data = []
        @items = nil
        @title = nil
        @vertical_label = nil
      end

      def count(time_span, last_update_time)
        counting = Count.new()
        @data.each do |datum|
          time =time_span.adjust_time(datum.time)

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
        rrd = rrd_name(time_span)
        last_update_time = ::RRD.last("#{rrd}") if File.exist?(rrd)

        data = collect_data(time_span, last_update_time)
        return if data.empty?

        end_time = data.last_time
        start_time = last_update_time ? last_update_time + time_span.step: data.first_time

        create_rrd(time_span, start_time, *@items) unless File.exist?(rrd)

        start_time.to_i.step(end_time, time_span.step) do |time|
          counts = []
          @items.each do |item|
            counts << data[item][time]
          end

          next if counts.uniq.length == 1 and counts.uniq.include?("U")

          p("update #{rrd} with #{Time.at(time)}  #{counts.join(':')}")
          ::RRD.update("#{rrd_name(time_span)}",
                       "#{time}:#{counts.join(':')}")
        end
      end

      def update
        collect
        update_db(TimeSpan.new("second"))
        update_db(TimeSpan.new("minute"))
        update_db(TimeSpan.new("hour"))
      end

      def create_rrd(time_span, start_time, *args)
        step = time_span.step
        rows = time_span.rows
        ::RRD.create("#{rrd_name(time_span)}",
            "--start", (start_time - 1).to_i.to_s,
            "--step", step,
            "RRA:MAX:0.5:1:#{rows}",
            "RRA:AVERAGE:0.5:1:#{rows}",
            *args.map{|arg| "DS:#{arg}:GAUGE:#{step}:0:U"})
      end

      def output_graph(time_span, start_time = nil, end_time = "now", width = 1000 , height = 250, *args)
        start_time = time_span.default_start_time unless start_time
        rrd_file = rrd_name(time_span)
        return unless File.exist?(rrd_file)
        last_update_time = ::RRD.last(rrd_file)

        time_stamp = last_update_time.strftime("%a %b %d %H:%M:%S %Z %Y")
        title = "#{@title} - #{time_stamp}"
        vertical_label = "#{@vertical_label}/#{time_span.short_name}"
        ::RRD.graph("#{graph_name(time_span)}",
                  "--title", title,
                  "--vertical-label", vertical_label,
                  "--step", time_span.step,
                  "--start", start_time,
                  "--end", end_time,
                  "--width","#{width}",
                  "--height", "#{height}",
                  "--alt-y-grid",
                  *(@items.map{|item| "DEF:#{item}=#{rrd_file}:#{item}:MAX"} +
                    @items.map{|item| "CDEF:n_#{item}=#{item},UN,0,#{item},IF"} +
                    args))
      end

      private
      def build_path(*paths)
        File.join(*([@rrd_directory, *paths].compact))
      end
    end

    class Session < Graph
      def initialize(rrd_directory, log, update_time)
        super(rrd_directory, log, update_time)
        @title = 'milter-manager condition'
        @vertical_label = "sessions"
        @items = ["smtp", "child"]
      end

      def rrd_name(time_span)
        build_path("milter-log.#{time_span.name}.rrd")
      end

      def graph_name(time_span)
        build_path("session.#{time_span.name}.png")
      end

      def collect_session(regex)
        sessions = []
        @log.each do |line|
          if Regexp.new(regex).match(line)
            time = Time.parse($1)
            name = $3
            id = $4
            if $2 == "Start"
              session = Milter::Session.new(id, name)
              session.start_time = time
              sessions << session
            else
              sessions.reverse_each do |session|
                if session.id == id
                  session.end_time = time
                  break
                end
              end
            end
          end
        end
        sessions
      end

      def collect
        @child_sessions =
          collect_session("milter-manager\\[.+\\]: \\[(.+)\\](Start|End).* filter process of (.*)\\((.+)\\)$")
        @client_sessions =
          collect_session("milter-manager\\[.+\\]: \\[(.+)\\](Start|End).* session in (.*)\\((.+)\\)$")
      end

      def count_sessions(sessions, time_span, last_update_time)
        counting = Hash.new("U")
        sessions.each do |session|
          next if session.end_time == nil
          start_time =time_span.adjust_time(session.start_time)
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
            count = counting[time]
            count = 0 if count == "U"
            count += 1
            counting[time] = count
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

      def output_graph(time_span, start_time = nil, end_time = "now", width = 1000 , height = 250)
        super(time_span, start_time, end_time, width, height,
              "LINE:n_smtp#0000ff:The number of SMTP session",
              "LINE:n_child#00ff00:The number of milter")
      end
    end

    class MailStatus < Graph
      def initialize(rrd_directory, log, update_time)
        super(rrd_directory, log, update_time)
        @vertical_label = "mails"
        @title = 'Processed mails'
        @items = ["normal", "accept",
                  "reject", "discard",
                  "temporary-failure",
                  "quarantine"]
      end

      def rrd_name(time_span)
        build_path("milter-log.mail.#{time_span.name}.rrd")
      end

      def graph_name(time_span)
        build_path("mail.#{time_span.name}.png")
      end

      def collect
        @log.each do |line|
          if /milter-manager\[.+\]: \[(.+)\]Reply (.+) to MTA on (.+)$/ =~ line
            time = Time.parse($1)
            status = $2
            state = $3
            next if state == "envelope-recipient"
            next if status == "continue" and state != "end-of-message"
            status = "normal" if status == "continue"
            @data << Milter::Mail.new(status, time)
          end
        end
      end

      def output_graph(time_span, start_time = nil, end_time = "now", width = 1000 , height = 250)
        super(time_span, start_time, end_time, width, height,
              "AREA:n_normal#0000ff:Normal",
              "STACK:n_accept#00ff00:Accept",
              "STACK:n_reject#ff0000:Reject",
              "STACK:n_discard#ffd400:Discard",
              "STACK:n_temporary-failure#888888:Temporary failure",
              "STACK:n_quarantine#a52a2a:Quarantine")
      end
    end

    class PassChild < Graph
      def initialize(rrd_directory, log, update_time)
        super(rrd_directory, log, update_time)
        @title = 'Pass child milters'
        @vertical_label = "milters"
        @items = ["connect",
                  "helo",
                  "envelope-from",
                  "envelope-recipient",
                  "header",
                  "body",
                  "end-of-message"]
      end

      def rrd_name(time_span)
        build_path("milter-log.state.#{time_span.name}.rrd")
      end

      def graph_name(time_span)
        build_path("pass-child.#{time_span.name}.png")
      end

      def collect
        @log.each do |line|
          if /milter-manager\[.+\]: \[(.+)\]Pass filter process of (.+)\(.+\) on (.+)$/ =~ line
            time = Time.parse($1)
            name = $2
            state = $3
            @data << Milter::PassChild.new(name, state, time)
          end
        end
      end

      def collect_data(time_span, last_update_time)
        pass_counting = count(time_span, last_update_time)
        Data.new(pass_counting)
      end

      def output_graph(time_span, start_time = nil, end_time = "now", width = 1000, height = 250)
        path = build_path("milter-log.#{time_span.name}.rrd")
        super(time_span, start_time, end_time, width, height,
              "AREA:n_connect#0000ff:connect",
              "STACK:n_helo#ff00ff:helo",
              "STACK:n_envelope-from#00ffff:envelope-from",
              "STACK:n_envelope-recipient#ffff00:envelope-recipient",
              "STACK:n_header#a52a2a:header",
              "STACK:n_body#ff0000:body",
              "STACK:n_end-of-message#00ff00:end-of-message",
              "DEF:child=#{path}:child:MAX",
              "CDEF:n_milters=child,UN,0,child,IF,10,/",
              "LINE:n_milters#000000:The number of 10 milters")
      end
    end
  end
end

# vi:ts=2:nowrap:ai:expandtab:sw=2
