#!/usr/bin/env ruby

require 'pathname'
require 'fileutils'
require 'time'
require 'optparse'

base = Pathname.new(__FILE__).dirname.expand_path
top = (base + "..").expand_path
$LOAD_PATH.unshift((top + "tool" + "ruby").to_s) #FIX Path
require 'milter-rrd'

class MilterLogTool
  def initialize
    @log = nil
    @update_db = true
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
        FileUtils.mkdir_p(@rrd_directory) unless File.exist?(@rrd_directory)
      end

      opts.on("--[no-]update-db",
              "Update RRD database with log file") do |boolean|
        @update_db = boolean
      end
    end
    opts.parse!(argv)
  end

  def update
    @now = Time.now.utc

    @sessions = Milter::RRD::Session.new(@rrd_directory, @log, @now)
    @sessions.update

    @mail_status = Milter::RRD::MailStatus.new(@rrd_directory, @log, @now)
    @mail_status.update
    
    @pass_child = Milter::RRD::PassChild.new(@rrd_directory, @log, @now)
    @pass_child.update
  end

  def output_graph(time_span)
    @sessions.output_graph(Milter::RRD::TimeSpan.new(time_span))
    @mail_status.output_graph(Milter::RRD::TimeSpan.new(time_span))
    @pass_child.output_graph(Milter::RRD::TimeSpan.new(time_span))
  end

  def output_all_graph
    output_graph("second")
    output_graph("minute")
    output_graph("hour")
  end
end

milter_log_tool = MilterLogTool.new
milter_log_tool.parse_options(ARGV)
milter_log_tool.update unless @update_rb
milter_log_tool.output_all_graph

# vi:ts=2:nowrap:ai:expandtab:sw=2
