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

class MilterMail
  attr_accessor :status, :time
  def initialize(status, time)
    @status = status
    @time = time
  end
end

class MilterChildPassData
  attr_accessor :name, :state, :time
  def initialize(name, state, time)
    @name = name
    @state = state
    @time = time
  end
end

class MilterStateCount < Hash
  def initialize
    self["connect"] = Hash.new("U")
    self["helo"] = Hash.new("U")
    self["envelope-from"] = Hash.new("U")
    self["envelope-recipient"] = Hash.new("U")
    self["header"] = Hash.new("U")
    self["body"] = Hash.new("U")
    self["end-of-message"] = Hash.new("U")
  end
  def [](state)
    super(state) ? super(state) : Hash.new("U")
  end
end

class MilterRRDData
  attr_accessor :amount
  def initialize(amount)
    @amount = amount
  end

  def empty?
    @amount.length == 0
  end

  def last_time
    if @amount.length > 0
      @amount.sort { |a, b| a[0] <=> b[0] }.last[0] + 1
    end
  end

  def first_time
    if @amount.length > 0
      @amount.sort { |a, b| a[0] <=> b[0] }.first[0]
    end
  end
end

class MilterRRDSessionData < MilterRRDData
  attr_accessor :child_counting
  def initialize(client_counting, child_counting)
    super(client_counting)
    @child_counting = child_counting
  end
end

class MilterRRDMailData < MilterRRDData
  attr_accessor :reject_mail_counting
  def initialize(mail_counting, reject_mail_counting)
    super(mail_counting)
    @reject_mail_counting = reject_mail_counting
  end
end

class MilterRRDChildPassData < MilterRRDData
  def initialize(child, filter_counting)
    super(child)
    @filter_counting = filter_counting
  end

  def [](state)
    @filter_counting[state]
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

class MilterLogTool
  def initialize
    @log = nil
    @child_sessions = nil
    @client_sessions = nil
    @reject_mails = nil
    @mails = nil
    @pass_filters = nil
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

  def state_rrd_name(time_span)
    "#{@rrd_directory}/milter-log.state.#{time_span.name}.rrd"
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

  def collect_mails(regex)
    mails = []
    @log.each do |line|
      if Regexp.new(regex).match(line)
        time = Time.parse($1)
        status = $2
        state = $3
        next if state == "envelope-recipient"
        mails << MilterMail.new(status, time)
      end
    end
    mails
  end

  def collect_pass_filters(regex)
    filters = []
    @log.each do |line|
      if Regexp.new(regex).match(line)
        time = Time.parse($1)
        name = $2
        state = $3
        filters << MilterChildPassData.new(name, state, time)
      end
    end
    filters
  end

  def collect_child_session
    @child_sessions = 
      collect_session("milter-manager\\[.+\\]: \\[(.+)\\](Start|End).* filter process of (.*)\\((.+)\\)$")
  end

  def collect_client_session
    @client_sessions = 
      collect_session("milter-manager\\[.+\\]: \\[(.+)\\](Start|End).* session in (.*)\\((.+)\\)$")
  end

  def collect_reject_mail
    @reject_mails =
      collect_mails("milter-manager\\[.+\\]: \\[(.+)\\]Reply (MILTER_STATUS_REJECT) to MTA on (.+)$")
  end

  def collect_mail
    @mails =
      collect_mails("milter-manager\\[.+\\]: \\[(.+)\\]Reply (.+) to MTA on (end-of-message)$")
  end

  def collect_pass_filter
    @pass_filters =
      collect_pass_filters("milter-manager\\[.+\\]: \\[(.+)\\]Pass filter process of (.+)\\(.+\\) on (.+)$")
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
      next if end_time >= time_span.adjust_time(@now)
      next if start_time >= time_span.adjust_time(@now)

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

  def count_mails(mails, time_span, last_update_time)
    counting = Hash.new("U")
    mails.each do |mail|
      time =time_span.adjust_time(mail.time)

      # ignore sessions which has been already registerd due to RRD.update fails on past time stamp
      if last_update_time
        next if time <= last_update_time
      end

      # ignore recent sessions due to RRD.update fails on past time stamp
      next if time >= time_span.adjust_time(@now)

      time = time.to_i

      count = counting[time]
      count = 0 if count == "U"
      count += 1
      counting[time] = count
    end
    counting
  end

  def count_pass_filters(pass_filters, time_span, last_update_time)
    counting = MilterStateCount.new()
    pass_filters.each do |pass_filter|
      time =time_span.adjust_time(pass_filter.time)

      # ignore sessions which has been already registerd due to RRD.update fails on past time stamp
      if last_update_time
        next if time <= last_update_time
      end

      # ignore recent sessions due to RRD.update fails on past time stamp
      next if time >= time_span.adjust_time(@now)

      time = time.to_i
      state = pass_filter.state

      count = counting[state][time]
      count = 0 if count == "U"
      count += 1
      counting[state][time] = count
    end
    counting
  end

  def collect_session_data(time_span, last_update_time)
    child_counting = count_sessions(@child_sessions, time_span, last_update_time)
    client_counting = count_sessions(@client_sessions, time_span, last_update_time)
    MilterRRDSessionData.new(client_counting, child_counting)
  end

  def collect_mail_data(time_span, last_update_time)
    mail_counting = count_mails(@mails, time_span, last_update_time)
    reject_mail_counting = count_mails(@reject_mails, time_span, last_update_time)
    MilterRRDMailData.new(mail_counting, reject_mail_counting)
  end

  def collect_pass_filter_data(time_span, last_update_time)
    child_counting = count_sessions(@child_sessions, time_span, last_update_time)
    pass_counting = count_pass_filters(@pass_filters, time_span, last_update_time)
    MilterRRDChildPassData.new(child_counting, pass_counting)
  end

  def create_state_distinct_rrd(time_span, start_time)
    step = time_span.step
    rows = time_span.rows
    RRD.create("#{rrd_name(time_span, "state")}",
        "--start", (start_time - 1).to_i.to_s,
        "--step", step,
        "DS:all:GAUGE:#{step}:0:U",
        "DS:connect:GAUGE:#{step}:0:U",
        "DS:helo:GAUGE:#{step}:0:U",
        "DS:envelope-from:GAUGE:#{step}:0:U",
        "DS:envelope-recipient:GAUGE:#{step}:0:U",
        "DS:header:GAUGE:#{step}:0:U",
        "DS:body:GAUGE:#{step}:0:U",
        "DS:end-of-message:GAUGE:#{step}:0:U",
        "RRA:MAX:0.5:1:#{rows}",
        "RRA:AVERAGE:0.5:1:#{rows}")
  end

  def create_time_span_rrd(time_span, start_time)
    step = time_span.step
    rows = time_span.rows
    RRD.create("#{rrd_name(time_span)}",
        "--start", (start_time - 1).to_i.to_s,
        "--step", step,
        "DS:client_sessions:GAUGE:#{step}:0:U",
        "DS:child_sessions:GAUGE:#{step}:0:U",
        "DS:mails:GAUGE:#{step}:0:U",
        "DS:reject_mails:GAUGE:#{step}:0:U",
        "RRA:MAX:0.5:1:#{rows}",
        "RRA:AVERAGE:0.5:1:#{rows}")
  end

  def update_state_db(time_span)
    last_update_time = RRD.last("#{state_rrd_name(time_span)}") if File.exist?(state_rrd_name(time_span))

    data = collect_pass_filter_data(time_span, last_update_time)
    return if data.empty?

    end_time = data.last_time
    start_time = last_update_time ? last_update_time + time_span.step : data.first_time

    create_state_distinct_rrd(time_span, start_time) unless File.exist?(state_rrd_name(time_span))
    start_time.to_i.step(end_time, time_span.step) do |time|
      RRD.update("#{state_rrd_name(time_span)}",
                 "#{time}" +
                 ":#{data.amount[time]}" +
                 ":#{data["connect"][time]}:#{data["helo"][time]}" +
                 ":#{data["envelope-from"][time]}:#{data["envelope-recipient"][time]}" +
                 ":#{data["header"][time]}:#{data["body"][time]}" +
                 ":#{data["end-of-message"][time]}")
    end
  end

  def update_db(time_span)
    last_update_time = RRD.last("#{rrd_name(time_span)}") if File.exist?(rrd_name(time_span))

    session_data = collect_session_data(time_span, last_update_time)
    return if session_data.empty?

    end_time = session_data.last_time
    start_time = last_update_time ? last_update_time + time_span.step: session_data.first_time

    create_time_span_rrd(time_span, start_time) unless File.exist?(rrd_name(time_span))

    mail_data = collect_mail_data(time_span, last_update_time)

    start_time.to_i.step(end_time, time_span.step) do |time|
      client_count = session_data.amount[time]
      child_count = session_data.child_counting[time]
      mail_count = mail_data.amount[time]
      reject_mail_count = mail_data.reject_mail_counting[time]

      if child_count == "U" and 
         client_count == "U" and
         mail_count == "U" and
         reject_mail_count == "U"
         next
      end

      p("update #{Time.at(time)}  #{client_count}:#{child_count}:#{mail_count}:#{reject_mail_count}")
      RRD.update("#{rrd_name(time_span)}",
                 "#{time}:#{client_count}:#{child_count}:#{mail_count}:#{reject_mail_count}")
    end
  end

  def update
    @now = Time.now.utc
    collect_client_session
    collect_child_session
    collect_reject_mail
    collect_mail
    collect_pass_filter
    update_db(MilterGraphTimeSpan.new("second"))
    update_db(MilterGraphTimeSpan.new("minute"))
    update_db(MilterGraphTimeSpan.new("hour"))

    update_state_db(MilterGraphTimeSpan.new("second"))
    update_state_db(MilterGraphTimeSpan.new("minute"))
    update_state_db(MilterGraphTimeSpan.new("hour"))
  end

  def output_session_graph(time_span, start_time = nil, end_time = "now", width = 1000 , height = 250)
    start_time = time_span.default_start_time unless start_time
    rrd_file = rrd_name(time_span)
    return unless File.exist?(rrd_file)
    RRD.graph("#{@rrd_directory}/session.#{time_span.name}.png",
              "--title", "milter-manager condition per #{time_span.name}",
              "DEF:client=#{rrd_file}:client_sessions:MAX",
              "DEF:child=#{rrd_file}:child_sessions:MAX",
              "CDEF:n_client=client,UN,0,client,IF",
              "CDEF:n_child=child,UN,0,child,IF",
              "LINE:n_client#0000ff:The number of SMTP session",
              "LINE:n_child#00ff00:The number of milter",
              "--step", time_span.step,
              "--start", start_time,
              "--end", end_time,
              "--width","#{width}",
              "--height", "#{height}",
              "--alt-y-grid",
              "COMMENT:Last update\\: #{@now.localtime.rfc2822.gsub!(/:/,'\\:')}\\r")
  end

  def output_mail_graph(time_span, start_time = nil, end_time = "now", width = 1000 , height = 250)
    start_time = time_span.default_start_time unless start_time
    rrd_file = rrd_name(time_span)
    return unless File.exist?(rrd_file)
    RRD.graph("#{@rrd_directory}/mail.#{time_span.name}.png",
              "--title", "Processed mails per #{time_span.name}",
              "DEF:all=#{rrd_file}:mails:MAX",
              "DEF:reject=#{rrd_file}:reject_mails:MAX",
              "CDEF:n_all=all,UN,0,all,IF",
              "CDEF:n_reject=reject,UN,0,reject,IF",
              "AREA:n_all#0000ff:The number of mails",
              "AREA:n_reject#ff0000:The number of rejected mails",
              "--step", time_span.step,
              "--start", start_time,
              "--end", end_time,
              "--width","#{width}",
              "--height", "#{height}",
              "--alt-y-grid",
              "COMMENT:Last update\\: #{@now.localtime.rfc2822.gsub!(/:/,'\\:')}\\r")
  end

  def output_pass_filter_graph(time_span, start_time = nil, end_time = "now", width = 1000, height = 250)
    start_time = time_span.default_start_time unless start_time
    rrd_file = state_rrd_name(time_span)
    return unless File.exist?(rrd_file)
    RRD.graph("#{@rrd_directory}/pass-filter.#{time_span.name}.png",
              "--title", "Processed mails per #{time_span.name}",
              "DEF:all=#{rrd_file}:all:MAX",
              "DEF:connect=#{rrd_file}:connect:MAX",
              "DEF:helo=#{rrd_file}:helo:MAX",
              "DEF:envelope-from=#{rrd_file}:envelope-from:MAX",
              "DEF:envelope-recipient=#{rrd_file}:envelope-recipient:MAX",
              "DEF:header=#{rrd_file}:header:MAX",
              "DEF:body=#{rrd_file}:body:MAX",
              "DEF:end-of-message=#{rrd_file}:end-of-message:MAX",
              "CDEF:n_all=all,UN,0,all,IF",
              "CDEF:n_connect=connect,UN,0,connect,IF",
              "CDEF:n_helo=helo,UN,0,helo,IF",
              "CDEF:n_envelope-from=envelope-from,UN,0,envelope-from,IF",
              "CDEF:n_envelope-recipient=envelope-recipient,UN,0,envelope-recipient,IF",
              "CDEF:n_header=header,UN,0,header,IF",
              "CDEF:n_body=body,UN,0,body,IF",
              "CDEF:n_end-of-message=end-of-message,UN,0,end-of-message,IF",
              "AREA:n_all#0000ff:The number of milters",
              "AREA:n_connect#ff0000:Passed on connect ",
              "STACK:n_helo#dd2200:Passed on helo ",
              "STACK:n_envelope-from#bb4400:Passed on envelope-from ",
              "STACK:n_envelope-recipient#996600:Passed on envelope-recipient ",
              "STACK:n_header#778800:Passed on header ",
              "STACK:n_body#55aa00:Passed on body ",
              "STACK:n_end-of-message#33cc00:Passed on end-of-message ",
              "--step", time_span.step,
              "--start", start_time,
              "--end", end_time,
              "--width","#{width}",
              "--height", "#{height}",
              "--alt-y-grid",
              "COMMENT:Last update\\: #{@now.localtime.rfc2822.gsub!(/:/,'\\:')}\\r")
  end

  def output_graph(time_span)
    output_session_graph(MilterGraphTimeSpan.new(time_span))
    output_mail_graph(MilterGraphTimeSpan.new(time_span))
    output_pass_filter_graph(MilterGraphTimeSpan.new(time_span))
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
