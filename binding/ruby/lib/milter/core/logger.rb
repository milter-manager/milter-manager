# Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

module Milter
  class Logger
    @@domain = "milter"
    class << self
      def domain
        @@domain
      end

      def domain=(domain)
        @@domain = domain
      end

      def error(message)
        default.log(:error, message, 1)
      end

      def critical(message)
        default.log(:critical, message, 1)
      end

      def message(message)
        default.log(:message, message, 1)
      end

      def warning(message)
        default.log(:warning, message, 1)
      end

      def debug(message)
        default.log(:debug, message, 1)
      end

      def info(message)
        default.log(:info, message, 1)
      end

      def trace(message)
        default.log(:trace, message, 1)
      end

      def statistics(message)
        default.log(:statistics, message, 1)
      end
    end

    def log(level, message, n_call_depth=nil)
      unless level.is_a?(Milter::LogLevelFlags)
        level = Milter::LogLevelFlags.const_get(level.to_s.upcase)
      end
      n_call_depth ||= 0
      file, line, info = caller[n_call_depth].split(/:(\d+):/, 3)
      ensure_message(message).each_line do |one_line_message|
        log_full(self.class.domain, level, file, line.to_i, info.to_s,
                 one_line_message.chomp)
      end
    end

    private
    def ensure_message(message)
      case message
      when nil
        ''
      when String
        message
      when Exception
        "#{message.class}: #{message.message}:\n#{message.backtrace.join("\n")}"
      else
        message.inspect
      end
    end
  end

  class ActiveRecordLogger
    module Severity
      DEBUG   = 0
      INFO    = 1
      WARN    = 2
      ERROR   = 3
      FATAL   = 4
      UNKNOWN = 5
    end
    include Severity

    def initialize(milter_logger)
      @milter_logger = milter_logger
    end

    def add(severity, message=nil, program_name=nil, n_call_depth=nil, &block)
      return unless need_log?(severity)
      message = (message || (block && block.call) || program_name).to_s
      @milter_logger.log(severity_to_level(severity), message,
                         n_call_depth || 1)
      message
    end

    def debug(message=nil, program_name=nil, &block)
      add(DEBUG, message, program_name, 2, &block)
    end

    def debug?
      need_log?(DEBUG)
    end

    def info(message=nil, program_name=nil, &block)
      add(INFO, message, program_name, 2, &block)
    end

    def info?
      need_log?(INFO)
    end

    def warn(message=nil, program_name=nil, &block)
      add(WARN, message, program_name, 2, &block)
    end

    def warn?
      need_log?(WARN)
    end

    def error(message=nil, program_name=nil, &block)
      add(ERROR, message, program_name, 2, &block)
    end

    def error?
      need_log?(ERROR)
    end

    def fatal(message=nil, program_name=nil, &block)
      add(FATAL, message, program_name, 2, &block)
    end

    def fatal?
      need_log?(FATAL)
    end

    def unknown(message=nil, program_name=nil, &block)
      add(UNKNOWN, message, program_name, 2, &block)
    end

    def unknown?
      need_log?(UNKNOWN)
    end

    private
    def need_log?(severity)
      @milter_logger.target_level >= severity_to_level(severity)
    end

    def severity_to_level(severity)
      case severity
      when DEBUG
        Milter::LOG_LEVEL_DEBUG
      when INFO
        Milter::LOG_LEVEL_INFO
      when WARN
        Milter::LOG_LEVEL_WARNING
      when ERROR
        Milter::LOG_LEVEL_ERROR
      when FATAL
        Milter::LOG_LEVEL_CRITICAL
      else
        Milter::LOG_LEVEL_NONE
      end
    end
  end
end
