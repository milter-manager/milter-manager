# Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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

require 'optparse'
require "ostruct"
require "webrick/server"

module Milter
  class Client
    class CommandLine
      attr_reader :options, :option_parser
      attr_accessor :name
      def initialize(options={})
        @name = options[:name] || File.basename($0, '.*')
        setup_options
        @option_parser = OptionParser.new(banner)
        yield(self) if block_given?
        setup_option_parser
      end

      def run(argv=nil)
        begin
          @option_parser.parse!(argv || ARGV)
        rescue
          puts $!.message
          puts @option_parser
          exit(false)
        end
        client = Milter::Client.new
        client.on_error do |_client, error|
          Milter::Logger.error("[client][error] #{error.message}")
        end
        client.start_syslog(@name) if @options.syslog
        client.status_on_error = @options.status_on_error
        client.connection_spec = @options.connection_spec
        client.effective_user = @options.user
        client.effective_group = @options.group
        client.unix_socket_group = @options.unix_socket_group
        if @options.unix_socket_mode
          client.unix_socket_mode = @options.unix_socket_mode
        end
        yield(client, options) if block_given?
        daemonize if @options.run_as_daemon
        if @options.n_workers
          i, o = UNIXSocket.pair
          @options.n_workers.times do
            child = fork do
              i.close
              client.fd_passing_io = o
              client.run_worker
            end
            Process.detach(child)
          end
          o.close
          client.fd_passing_io = i
          client.run_master
        else
          client.main
        end
      end

      private
      def setup_options
        @options = OpenStruct.new
        @options.connection_spec = "inet:20025"
	@options.status_on_error = "accept"
        @options.run_as_daemon = false
        @options.user = nil
        @options.group = nil
        @options.unix_socket_group = nil
        @options.unix_socket_mode = nil
        @options.syslog = false
      end

      def setup_option_parser
        setup_common_options
      end

      def banner
        "Usage: %s [options]" % File.basename($0, '.*')
      end

      def setup_common_options
        setup_basic_options
        setup_milter_options
        setup_logger_options
      end

      def setup_basic_options
        @option_parser.separator ""
        @option_parser.separator "Basic options"

        @option_parser.on("--help", "Show this message.") do
          puts @option_parser
          exit(true)
        end

        @option_parser.on("--library-version",
                          "Show milter library version.") do
          puts Milter::TOOLKIT_VERSION.join(".")
          exit(true)
        end
      end

      def setup_milter_options
        @option_parser.separator ""
        @option_parser.separator "milter options"

        @option_parser.on("-s", "--connection-spec=SPEC",
                          "Specify connection spec as [SPEC].",
                          "(#{@options.connection_spec})") do |spec|
          @options.connection_spec = spec
        end

        @option_parser.on("--[no-]daemon",
                          "Run as a daemon process.",
                          "(#{@options.run_as_daemon})") do |run_as_daemon|
          @options.run_as_daemon = run_as_daemon
        end

        statuses = ["accept", "reject", "temporary_failure"]
        @option_parser.on("--status-on-error=STATUS",
                          statuses,
                          "Specify status on error.",
                          "(#{@options.status_on_error})") do |status|
          @options.status_on_error = status
        end

        @option_parser.on("--user=USER",
                          "Run as USER's process (need root privilege)",
                          "(#{@options.user})") do |user|
          @options.user = user
        end

        @option_parser.on("--group=GROUP",
                          "Run as GROUP's process (need root privilege)",
                          "(#{@options.group})") do |group|
          @options.group = group
        end

        @option_parser.on("--unix-socket-group=GROUP",
                          "Change UNIX domain socket group to GROUP",
                          "(#{@options.unix_socket_group})") do |group|
          @options.unix_socket_group = group
        end

        client = Milter::Client.new
        @option_parser.on("--unix-socket-mode=MODE",
                          "Change UNIX domain socket mode to MODE",
                          "(#{client.default_unix_socket_mode})") do |mode|
          client.unix_socket_mode = mode
          @options.unix_socket_mode = mode
        end

        @option_parser.on("--n-workers=<NUMBER-OF-WORKERS>",
                          Integer,
                          "Run in multi workers mode") do |num|
          if num <= 0
            raise OptionParser::InvalidArgument
          end
          @options.n_workers = num
        end
      end

      def setup_logger_options
        @option_parser.separator ""
        @option_parser.separator "Logging options"

        level_names = Milter::LogLevelFlags.values.collect {|value| value.nick}
        level_names << "all"
        @option_parser.on("--log-level=LEVEL",
                          "Specify log level as [LEVEL].",
                          "Select from [%s]." % level_names.join(', '),
                          "(#{ENV['MILTER_LOG_LEVEL'] || 'none'})") do |level|
          if level.empty?
            ENV["MILTER_LOG_LEVEL"] = nil
          else
            ENV["MILTER_LOG_LEVEL"] = level
          end
        end

        @option_parser.on("--[no-]syslog",
                          "Use syslog",
                          "(#{@options.syslog})") do |bool|
          @options.syslog = bool
        end

        @option_parser.on("--verbose",
                          "Show messages verbosely.",
                          "Alias of --log-level=all.") do
          ENV["MILTER_LOG_LEVEL"] = "all"
        end
      end

      def daemonize
        WEBrick::Daemon.start
      end
    end
  end
end
