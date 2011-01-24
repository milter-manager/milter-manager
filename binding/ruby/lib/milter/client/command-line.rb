# Copyright (C) 2010-2011  Kouhei Sutou <kou@clear-code.com>
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
require "syslog"
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
        client.start_syslog(@name, @options.syslog_facility) if @options.syslog
        client.status_on_error = @options.status_on_error
        client.connection_spec = @options.connection_spec
        client.effective_user = @options.user
        client.effective_group = @options.group
        client.unix_socket_group = @options.unix_socket_group
        if @options.unix_socket_mode
          client.unix_socket_mode = @options.unix_socket_mode
        end
        client.event_loop_backend = @options.event_loop_backend
        client.default_packet_buffer_size = @options.packet_buffer_size
        client.n_workers = @options.n_workers
        yield(client, options) if block_given?
        daemonize if @options.run_as_daemon
        setup_signal_handler(client) if @options.handle_signal
        client.run
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
        @options.syslog_facility = "mail"
        @options.event_loop_backend = Milter::Client::EVENT_LOOP_BACKEND_GLIB
        @options.n_workers = 0
        @options.packet_buffer_size = 0
        @options.handle_signal = true
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
                          "(%o)" % client.default_unix_socket_mode) do |mode|
          client.unix_socket_mode = mode
          @options.unix_socket_mode = mode
        end

        backends = Milter::ClientEventLoopBackend.values.collect do |value|
          value.nick
        end
        @option_parser.on("--event-loop-backend=BACKEND", backends,
                          "Use BACKEND as event loop backend.",
                          "available values: [#{backends.join(', ')}]",
                          "(#{@options.event_loop_backend.nick})") do |backend|
          @options.event_loop_backend = backend
        end

        @option_parser.on("--n-workers=N",
                          Integer,
                          "Run with N workrs.",
                          "(#{@options.n_workers})") do |num|
          if num <= 0
            raise OptionParser::InvalidArgument
          end
          @options.n_workers = num
        end

        @option_parser.on("--packet-buffer-size=SIZE",
                          Integer,
                          "Use SIZE as packet buffer size.",
                          "(#{@options.packet_buffer_size})") do |size|
          if size < 0
            raise OptionParser::InvalidArgument
          end
          @options.packet_buffer_size = size
        end

        @option_parser.on("--[no-]handle-signal",
                          "Handle SIGHUP, SIGINT and SIGTERM signals",
                          "(#{@options.handle_signal})") do |boolean|
          @options.handle_signal = boolean
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

        facilities = Syslog.constants.find_all do |name|
          /\ALOG_/ =~ name.to_s
        end.collect do |name|
          name.to_s.gsub(/\ALOG_/, '').downcase
        end
        available_values = "available values: [#{facilities.join(', ')}]"
        @option_parser.on("--syslog-facility=FACILITY", facilities,
                          "Use FACILITY as syslog facility.",
                          "(#{@options.syslog_facility})",
                          available_values) do |facility|
          @options.syslog_facility = facility
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

      def setup_signal_handler(client)
        trap(:HUP) {client.reload}
        default_sigint_handler = trap(:INT) do
          client.shutdown
          trap(:INT, default_sigint_handler)
        end
        default_sigterm_handler = trap(:TERM) do
          client.shutdown
          trap(:TERM, default_sigterm_handler)
        end
      end
    end
  end
end
