#!/usr/bin/env ruby

require 'RRD'
require 'time'
require 'optparse'

class MilterRRDCount < Hash
  def [](key)
    super(key) ? super(key) : store(key, Hash.new("U"))
  end
end

class MilterRRDData
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

class MilterGraphTimeSpan
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

class MilterRRD
  def initialize(rrd_directory, log, update_time)
    @rrd_directory = rrd_directory
    @log = log
    @update_time = update_time
    @data = []
    @items = nil
    @title = nil
  end

  def count(time_span, last_update_time)
    counting = MilterRRDCount.new()
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
    MilterRRDData.new(counting)
  end

  def update_db(time_span)
    rrd = rrd_name(time_span)
    last_update_time = RRD.last("#{rrd}") if File.exist?(rrd)

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
      RRD.update("#{rrd_name(time_span)}",
                 "#{time}:#{counts.join(':')}")
    end
  end

  def update
    collect
    update_db(MilterGraphTimeSpan.new("second"))
    update_db(MilterGraphTimeSpan.new("minute"))
    update_db(MilterGraphTimeSpan.new("hour"))
  end

  def create_rrd(time_span, start_time, *args)
    step = time_span.step
    rows = time_span.rows
    RRD.create("#{rrd_name(time_span)}",
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
    RRD.graph("#{graph_name(time_span)}",
              "--title", "#{@title} per #{time_span.name}",
              "--step", time_span.step,
              "--start", start_time,
              "--end", end_time,
              "--width","#{width}",
              "--height", "#{height}",
              "--alt-y-grid",
              "COMMENT:Last update\\: #{@update_time.localtime.rfc2822.gsub!(/:/,'\\:')}\\r",
              *(@items.map{|item| "DEF:#{item}=#{rrd_file}:#{item}:MAX"} +
                @items.map{|item| "CDEF:n_#{item}=#{item},UN,0,#{item},IF"} +
                args))
  end
end

class MilterSessionRRD < MilterRRD
  class MilterSession
    attr_accessor :id, :name, :start_time, :end_time
    def initialize(id, name)
      @id = id
      @name = name
      @start_time = nil
      @end_time = nil
    end
  end

  def initialize(rrd_directory, log, update_time)
    super(rrd_directory, log, update_time)
    @items = ["smtp", "child"]
    @title = 'milter-manager condition'
  end

  def rrd_name(time_span)
    "#{@rrd_directory}/milter-log.#{time_span.name}.rrd"
  end

  def graph_name(time_span)
    "#{@rrd_directory}/session.#{time_span.name}.png"
  end

  def collect_session(regex)
    sessions = []
    @log.each do |line|
      if Regexp.new(regex).match(line)
        time = Time.parse($1)
        name = $3
        id = $4
        if $2 == "Start"
          session = MilterSession.new(id, name)
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
    counting = MilterRRDCount.new()
    counting["smtp"] = count_sessions(@client_sessions, time_span, last_update_time)
    counting["child"] = count_sessions(@child_sessions, time_span, last_update_time)
    MilterRRDData.new(counting)
  end

  def output_graph(time_span, start_time = nil, end_time = "now", width = 1000 , height = 250)
    super(time_span, start_time, end_time, width, height,
          "LINE:n_smtp#0000ff:The number of SMTP session",
          "LINE:n_child#00ff00:The number of milter")
  end
end

class MilterMailStatusRRD < MilterRRD
  class MilterMail
    attr_accessor :status, :time, :key
    def initialize(status, time)
      @key = status
      @status = status
      @time = time
    end
  end

  def initialize(rrd_directory, log, update_time)
    super(rrd_directory, log, update_time)
    @items = ["normal", "accept",
              "reject", "discard",
              "temporary-failure",
              "quarantine"]
    @title = 'Processed mails'
  end

  def rrd_name(time_span)
    "#{@rrd_directory}/milter-log.mail.#{time_span.name}.rrd"
  end

  def graph_name(time_span)
    "#{@rrd_directory}/mail.#{time_span.name}.png"
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
        @data << MilterMail.new(status, time)
      end
    end
  end

  def output_graph(time_span, start_time = nil, end_time = "now", width = 1000 , height = 250)
    super(time_span, start_time, end_time, width, height,
          "AREA:n_normal#0000ff:Normal mails",
          "STACK:n_accept#00ff00:Accept mails",
          "STACK:n_reject#ff0000:Reject mails",
          "STACK:n_discard#ffd400:Discard mails",
          "STACK:n_temporary-failure#888888:Temporary failure mails",
          "STACK:n_quarantine#a52a2a:Quarantine mails")
  end
end

class MilterPassChildRRD < MilterRRD
  class MilterPassChildData
    attr_accessor :name, :state, :time, :key
    def initialize(name, state, time)
      @name = name
      @state = state
      @time = time
      @key = state
    end
  end

  def initialize(rrd_directory, log, update_time)
    super(rrd_directory, log, update_time)
    @title = 'Pass child milters'
    @items = ["all",
              "connect",
              "helo",
              "envelope-from",
              "envelope-recipient",
              "header",
              "body",
              "end-of-message"]
  end

  def rrd_name(time_span)
    "#{@rrd_directory}/milter-log.state.#{time_span.name}.rrd"
  end

  def graph_name(time_span)
    "#{@rrd_directory}/pass-child.#{time_span.name}.png"
  end

  def collect
    @log.each do |line|
      if /milter-manager\[.+\]: \[(.+)\]Pass filter process of (.+)\(.+\) on (.+)$/ =~ line
        time = Time.parse($1)
        name = $2
        state = $3
        @data << MilterPassChildData.new(name, state, time)
      end
    end
  end

  def collect_data(time_span, last_update_time)
    pass_counting = count(time_span, last_update_time)
#    pass_counting["all"] = count_sessions(@child_sessions, time_span, last_update_time)
    MilterRRDData.new(pass_counting)
  end

  def output_graph(time_span, start_time = nil, end_time = "now", width = 1000, height = 250)
    super(time_span, start_time, end_time, width, height,
          "AREA:n_all#00ff00:The number of milters",
          "AREA:n_connect#0000ff:Passed on connect",
          "STACK:n_helo#0022dd:Passed on helo",
          "STACK:n_envelope-from#0044bb:Passed on envelope-from",
          "STACK:n_envelope-recipient#006699:Passed on envelope-recipient",
          "STACK:n_header#008877:Passed on header",
          "STACK:n_body#00aa55:Passed on body",
          "STACK:n_end-of-message#00cc33:Passed on end-of-message")
  end
end

class MilterLogTool
  def initialize
    @log = nil
    @update_db = false
    @sessions = nil
    @mail_status = nil
    @pass_child = nil
  end

  def parse_options(argv)
    opts = OptionParser.new do |opts|
      opts.on("--log=LOG_FILENAME", "The log file name in which is stored Milter log") do |log|
        @log = File.read(log)
      end

      opts.on("--rrd-directory=DIRECTORY") do |directory|
        @rrd_directory = directory
        Dir.mkdir(@rrd_directory) unless File.exist?(@rrd_directory)
      end

      opts.on("--update-db",
              "Update RRD database with log file") do |boolean|
        @update_db = boolean
      end
    end
    opts.parse!(argv)
  end

  def update
    @now = Time.now.utc

    @sessions = MilterSessionRRD.new(@rrd_directory, @log, @now)
    @sessions.update

    @mail_status = MilterMailStatusRRD.new(@rrd_directory, @log, @now)
    @mail_status.update
    
    @pass_child = MilterPassChildRRD.new(@rrd_directory, @log, @now)
    @pass_child.update
  end

  def output_graph(time_span)
    @sessions.output_graph(MilterGraphTimeSpan.new(time_span))
    @mail_status.output_graph(MilterGraphTimeSpan.new(time_span))
    @pass_child.output_graph(MilterGraphTimeSpan.new(time_span))
  end

  def output_all_graph
    output_graph("second")
    output_graph("minute")
    output_graph("hour")
  end
end

milter_log_tool = MilterLogTool.new
milter_log_tool.parse_options(ARGV)
milter_log_tool.update
milter_log_tool.output_all_graph

# vi:ts=2:nowrap:ai:expandtab:sw=2
