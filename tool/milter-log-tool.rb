#!/usr/bin/env ruby

require 'RRD'
require 'time'
require 'optparse'

class MilterSession
  attr_accessor :id, :name, :start_time, :end_time
  def initialize(id, name)
    @id = id
    @name = name
    @start_time = nil
    @end_time = nil
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
      "-1h" # 1hour
    when "minute"
      "-12h" # 12hours
    when "hour"
      "-1d" # 1day
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

class MilterLogTool
  def initialize
    @log = nil
    @child_sessions = nil
    @client_sessions = nil
    @update_db = false
    @now
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

  def rrd_name(time_span)
    "#{@rrd_directory}/milter-log.#{time_span.name}.rrd"
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

  def collect_client_session
    @client_sessions = 
      collect_session("milter-manager\\[.+\\]: \\[(.+)\\](Start|End).* filter process of (.*)\\((.+)\\)$")
  end

  def collect_child_session
    @child_sessions = 
      collect_session("milter-manager\\[.+\\]: \\[(.+)\\](Start|End).* session in (.*)\\((.+)\\)$")
  end

  def count_sessions(sessions, time_span, last_update_time)
    counting = Hash::new
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
      next if end_time >= time_span.adjust_time(@now)
      next if start_time >= time_span.adjust_time(@now)

      start_time = start_time.to_i
      end_time = end_time.to_i

      start_time.step(end_time, time_span.step) do |time|
        count = counting[time] ? counting[time] : 0
        count += 1
        counting[time] = count
      end
    end
    counting
  end

  def collect_data(time_span, last_update_time)
    child_counting = count_sessions(@child_sessions, time_span, last_update_time)
    client_counting = count_sessions(@client_sessions, time_span, last_update_time)
    [child_counting, client_counting]
  end

  def create_time_span_rrd(time_span, start_time)
    step = time_span.step
    rows = time_span.rows
    RRD.create("#{rrd_name(time_span)}",
        "--start", (start_time - 1).to_i.to_s,
        "--step", step,
        "DS:child_sessions:GAUGE:#{step}:0:U",
        "DS:client_sessions:GAUGE:#{step}:0:U",
        "RRA:MAX:0.5:1:#{rows}",
        "RRA:AVERAGE:0.5:1:#{rows}")
  end

  def update_db(time_span)
    last_update_time = RRD.last("#{rrd_name(time_span)}") if File.exist?(rrd_name(time_span))

    counting = collect_data(time_span, last_update_time)

    if counting[0].length > 0
      end_time = counting[0].sort { |a, b| a[0] <=> b[0] }.last[0] + 1
    elsif counting[1].length > 0
      end_time  = counting[1].sort { |a, b| a[0] <=> b[0] }.last[0] + 1
    else
      return
    end

    start_time = counting[0].sort { |a, b| a[0] <=> b[0] }.first[0]
    create_time_span_rrd(time_span, start_time) unless File.exist?(rrd_name(time_span))

    start_time.to_i.step(end_time, time_span.step) do |time|
      child_count = counting[1][time] ? counting[1][time] : 0
      client_count = counting[0][time] ? counting[0][time] : 0
      p("update #{Time.at(time)}  #{child_count}:#{client_count}")
      RRD.update("#{rrd_name(time_span)}",
                 "#{time}:#{child_count}:#{client_count}")
    end
  end

  def update
    @now = Time.now.utc
    collect_client_session
    collect_child_session
    update_db(MilterGraphTimeSpan.new("second"))
    update_db(MilterGraphTimeSpan.new("minute"))
    update_db(MilterGraphTimeSpan.new("hour"))
  end

  def output_graph(time_span, start_time = nil, end_time = "now", width = 1000 , height = 250)
    start_time = time_span.default_start_time unless start_time
    RRD.graph("#{@rrd_directory}/#{time_span.name}.png",
              "--title", "per #{time_span.name}",
              "DEF:client=#{rrd_name(time_span)}:client_sessions:MAX",
              "DEF:child=#{rrd_name(time_span)}:child_sessions:MAX",
              "LINE1:child#00ff00:The number of milter",
              "LINE2:client#ff0000:The number of SMTP session",
              "--step", time_span.step,
              "--start", start_time,
              "--end", end_time,
              "--width","#{width}",
              "--height", "#{height}")
  end

  def output_all_graph
    output_graph(MilterGraphTimeSpan.new("second"))
    output_graph(MilterGraphTimeSpan.new("minute"))
    output_graph(MilterGraphTimeSpan.new("hour"))
  end
end

milter_log_tool = MilterLogTool.new
milter_log_tool.parse_options(ARGV)
milter_log_tool.update
milter_log_tool.output_all_graph

# vi:ts=2:nowrap:ai:expandtab:sw=2
