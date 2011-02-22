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
      attr_reader :options, :configuration, :option_parser
      def initialize(options={})
        setup_options
        setup_configuration(options)
        @option_parser = OptionParser.new(banner)
        yield(self) if block_given?
        setup_option_parser
      end

      def run(argv=nil)
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
        client = Milter::Client.new
        client.on_error do |_client, error|
          Milter::Logger.error("[client][error] #{error.message}")
        end
        yield(client, options) if block_given?
        @configuration.setup(client)
        if @configuration.milter.pid_file
          begin
            pid_file = open(@configuration.milter.pid_file, "w")
          rescue => error
            Milter::Logger.error("[pid_file][error] #{error.message}")
            exit(false)
          end
        end
        client.listen
        client.drop_privilege
        daemonize if @configuration.milter.daemon?
        if pid_file
          pid_file.puts($$)
          pid_file.close
        end
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
        @configuration.milter.name = options[:name] || File.basename($0, '.*')
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
          puts Milter::TOOLKIT_VERSION.join(".")
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
        @option_parser.on("-s", "--connection-spec=SPEC",
                          "Specify connection spec as [SPEC].",
                          "(#{milter_conf})") do |spec|
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

        statuses = ["accept", "reject", "temporary_failure"]
        @option_parser.on("--status-on-error=STATUS",
                          statuses,
                          "Specify status on error.",
                          "(#{milter_conf.status_on_error})") do |status|
          milter_conf.status_on_error = status
        end

        @option_parser.on("--user=USER",
                          "Run as USER's process (need root privilege)",
                          "(#{milter_conf.user})") do |user|
          milter_conf.user = user
        end

        @option_parser.on("--group=GROUP",
                          "Run as GROUP's process (need root privilege)",
                          "(#{milter_conf.group})") do |group|
          milter_conf.group = group
        end

        @option_parser.on("--unix-socket-group=GROUP",
                          "Change UNIX domain socket group to GROUP",
                          "(#{milter_conf.unix_socket_group})") do |group|
          milter_conf.unix_socket_group = group
        end

        client = Milter::Client.new
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

        milter_conf = @configuration.milter
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
                          "(#{milter_conf.use_syslog?})") do |bool|
          milter_conf.use_syslog = bool
        end

        facilities = Syslog.constants.find_all do |name|
          /\ALOG_/ =~ name.to_s
        end.collect do |name|
          name.to_s.gsub(/\ALOG_/, '').downcase
        end
        available_values = "available values: [#{facilities.join(', ')}]"
        @option_parser.on("--syslog-facility=FACILITY", facilities,
                          "Use FACILITY as syslog facility.",
                          "(#{milter_conf.syslog_facility})",
                          available_values) do |facility|
          milter_conf.syslog_facility = facility
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
