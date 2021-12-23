# Copyright (C) 2010-2021  Sutou Kouhei <kou@clear-code.com>
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

require "optparse"
require "ostruct"
require "syslog"

module Milter
  class Client
    class CommandLine
      attr_reader :options, :configuration, :option_parser
      def initialize(options={})
        setup_options
        setup_configuration(options)
        @option_parser = OptionParser.new(banner)
        yield(self) if block_given?
        setup_option_parser
      end

      def parse(argv=nil)
        begin
          @option_parser.parse!(argv || ARGV)
        rescue OptionParser::ParseError
          puts($!.message)
          exit(false)
        rescue
          puts("#{$!.class}: #{$!.message}")
          puts($@)
          exit(false)
        end
      end

      def run(argv=nil)
        parse(argv)
        client = Milter::Client.new
        client.on_error do |_client, error|
          Milter::Logger.error("[client][error] #{error.message}")
        end
        @configuration.setup(client)
        client.event_loop = client.create_event_loop(true)
        yield(client, options) if block_given?
        client.listen
        client.drop_privilege
        client.daemonize if @configuration.milter.daemon?
        setup_signal_handler(client) if @configuration.milter.handle_signal?
        client.run
      end

      def name
        @configuration.milter.name
      end

      private
      def setup_options
        @options = OpenStruct.new
      end

      def setup_configuration(options)
        @configuration = Configuration.new
        @configuration.milter.name = options[:name] || File.basename($0, ".*")
      end

      def setup_option_parser
        setup_common_options
      end

      def banner
        "Usage: %s [options]" % @configuration.milter.name
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
          puts Milter::VERSION.join(".")
          exit(true)
        end

        @option_parser.on("-cFILE", "--configuration=FILE",
                          "Load configuration from FILE") do |path|
          loader = ConfigurationLoader.new(@configuration)
          loader.load(path)
        end

        @option_parser.on("-eENV", "--environment=ENV",
                          "Set milter environment as ENV",
                          "(#{@configuration.environment})") do |env|
          @configuration.environment = env
        end
      end

      def setup_milter_options
        @option_parser.separator ""
        @option_parser.separator "milter options"

        milter_conf = @configuration.milter
        security_conf = @configuration.security
        @option_parser.on("-s", "--connection-spec=SPEC",
                          "Specify connection spec as [SPEC].",
                          "(#{milter_conf.connection_spec})") do |spec|
          milter_conf.connection_spec = spec
        end

        @option_parser.on("--[no-]daemon",
                          "Run as a daemon process.",
                          "(#{milter_conf.daemon?})") do |run_as_daemon|
          milter_conf.daemon = run_as_daemon
        end

        @option_parser.on("--pid-file=FILE",
                          "Write process ID to FILE.") do |pid_file|
          milter_conf.pid_file = pid_file
        end

        loader = ConfigurationLoader::MilterConfigurationLoader.new(loader)
        available_statues = loader.available_fallback_statuses
        available_values = "available values: [#{available_statues.join(', ')}]"
        @option_parser.on("--fallback-status=STATUS",
                          available_statues,
                          "Specify status on error.",
                          available_values,
                          "(#{milter_conf.fallback_status})") do |status|
          milter_conf.fallback_status = status
        end

        @option_parser.on("--status-on-error=STATUS",
                          "Deprecated option.",
                          "Use --fallback-status instead.") do |status|
          milter_conf.fallback_status = status
        end

        @option_parser.on("--user=USER",
                          "Run as USER's process (need root privilege)",
                          "(#{security_conf.effective_user})") do |user|
          security_conf.effective_user = user
        end

        @option_parser.on("--group=GROUP",
                          "Run as GROUP's process (need root privilege)",
                          "(#{security_conf.effective_group})") do |group|
          security_conf.effective_group = group
        end

        @option_parser.on("--unix-socket-group=GROUP",
                          "Change UNIX domain socket group to GROUP",
                          "(#{milter_conf.unix_socket_group})") do |group|
          milter_conf.unix_socket_group = group
        end

        @option_parser.on("--unix-socket-mode=MODE",
                          "Change UNIX domain socket mode to MODE",
                          "(%o)" % milter_conf.unix_socket_mode) do |mode|
          milter_conf.unix_socket_mode = mode
        end

        @option_parser.on("--max-file-descriptors=NUM",
                          "Change maximum number of file descriptors to NUM",
                          Integer) do |num|
          milter_conf.max_file_descriptors = num
        end

        backends = Milter::ClientEventLoopBackend.values.collect do |value|
          value.nick
        end
        @option_parser.on("--event-loop-backend=BACKEND", backends,
                          "Use BACKEND as event loop backend.",
                          "available values: [#{backends.join(', ')}]",
                          "(#{milter_conf.event_loop_backend})") do |backend|
          milter_conf.event_loop_backend = backend
        end

        @option_parser.on("--n-workers=N",
                          Integer,
                          "Run with N workrs.",
                          "(#{milter_conf.n_workers})") do |n|
          raise OptionParser::InvalidArgument if n <= 0
          milter_conf.n_workers = n
        end

        @option_parser.on("--packet-buffer-size=SIZE",
                          Integer,
                          "Use SIZE as packet buffer size.",
                          "(#{milter_conf.packet_buffer_size})") do |size|
          raise OptionParser::InvalidArgument if size < 0
          milter_conf.packet_buffer_size = size
        end

        @option_parser.on("--max-pending-finished-sessions=N",
                          Integer,
                          "Don't hold over N pending finished sessions.",
                          "(#{milter_conf.max_pending_finished_sessions})") do |n|
          raise OptionParser::InvalidArgument if n <= 0
          milter_conf.max_pending_finished_sessions = n
        end

        @option_parser.on("--[no-]handle-signal",
                          "Handle SIGHUP, SIGINT and SIGTERM signals",
                          "(#{milter_conf.handle_signal?})") do |boolean|
          milter_conf.handle_signal = boolean
        end

        @option_parser.on("--maintenance-interval=N",
                          "Run maintenance callback after each N sessions",
                          "(#{milter_conf.maintenance_interval})") do |interval|
          milter_conf.maintenance_interval = interval
        end

        @option_parser.on("--[no-]run-gc-on-maintain",
                          "Run GC in each maintenance callback",
                          "(#{milter_conf.run_gc_on_maintain?})") do |boolean|
          milter_conf.run_gc_on_maintain = boolean
        end
      end

      def setup_logger_options
        @option_parser.separator ""
        @option_parser.separator "Logging options"

        log_conf = @configuration.log
        level_names = Milter::LogLevelFlags.values.collect {|value| value.nick}
        level_names << "all"
        @option_parser.on("--log-level=LEVEL",
                          "Specify log level as LEVEL.",
                          "Select from [%s]." % level_names.join(", "),
                          "(#{ENV['MILTER_LOG_LEVEL'] || 'default'})") do |level|
          log_conf.level = level
        end

        @option_parser.on("--log-path=PATH",
                          "Specify log output path to PATH.",
                          "If PATH is '-', the standard output is used",
                          "(The standard output)") do |path|
          log_conf.path = path
        end

        @option_parser.on("--[no-]syslog",
                          "Use syslog",
                          "(#{log_conf.use_syslog?})") do |bool|
          log_conf.use_syslog = bool
        end

        facilities = Syslog.constants.find_all do |name|
          /\ALOG_/ =~ name.to_s
        end.collect do |name|
          name.to_s.gsub(/\ALOG_/, "").downcase
        end
        available_values = "available values: [#{facilities.join(', ')}]"
        @option_parser.on("--syslog-facility=FACILITY", facilities,
                          "Use FACILITY as syslog facility.",
                          "(#{log_conf.syslog_facility})",
                          available_values) do |facility|
          log_conf.syslog_facility = facility
        end

        @option_parser.on("--verbose",
                          "Show messages verbosely.",
                          "Alias of --log-level=all.") do
          log_conf.level = "all"
        end
      end

      def setup_signal_handler(client)
        # FIXME: This is just a workaround to handle signals within 0.5 seconds.
        # We should use more clever approach instead of this.
        if client.event_loop_backend == Milter::ClientEventLoopBackend::LIBEV
          client.event_loop.add_timeout(0.5) do
            true
          end
        end

        if client.n_workers == 0
          setup_signal_handler_reload(client)
          setup_signal_handler_reopen_log(client)
          setup_signal_handler_shutdown(client)
        end

        client.signal_connect("worker-created") do
          setup_signal_handler_reload(client)
          setup_signal_handler_reopen_log(client)
        end

        client.signal_connect("workers-created") do
          setup_signal_handler_reload(client) do
            Process.kill(:HUP, *client.worker_pids)
          end
          setup_signal_handler_reopen_log(client) do
            Process.kill(:USR1, *client.worker_pids)
          end
          setup_signal_handler_shutdown(client)
        end
      end

      def setup_signal_handler_reload(client)
        add_signal_handler(client, :HUP) do
          client.reload
          yield(client) if block_given?
        end
      end

      def setup_signal_handler_reopen_log(client)
        add_signal_handler(client, :USR1) do
          Milter::Logger.default.reopen
          yield(client) if block_given?
        end
      end

      def setup_signal_handler_shutdown(client)
        add_signal_handler(client, :INT,  :once => true) {client.shutdown}
        add_signal_handler(client, :TERM, :once => true) {client.shutdown}
      end

      def add_signal_handler(client, signal, options={})
        notification_message = "N"
        notification_in, notification_out = IO.pipe
        notification_in_channel = GLib::IOChannel.new(notification_in)

        event_loop = client.event_loop
        event_loop.watch_io(notification_in_channel, GLib::IOCondition::IN) do
          notification_in.read(notification_message.size)
          yield
          if options[:once]
            notification_in.close
            notification_out.close
            continue_p = false
          else
            continue_p = true
          end
          continue_p
        end

        default_handler = trap(signal) do
          notification_out.write(notification_message)
          notification_out.flush
          trap(signal, default_handler) if options[:once]
        end
      end
    end
  end
end
