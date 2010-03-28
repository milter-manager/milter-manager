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

module Milter::Manager
  class Breaker
    attr_writer :threshold_n_connections
    def initialize
      @threshold_n_connections = nil
    end

    def stressing?(context)
      threshold = threshold_n_connections
      return false if threshold.zero?
      threshold <= context.n_processing_sessions
    end

    def threshold_n_connections
      @threshold_n_connections ||= detect_threshold_n_connections
    end

    private
    def detect_threshold_n_connections
      threshold = 0
      log_tag = "[breaker][detect][threshold]"
      if postfix?
        threshold = detect_postfix_threshold_n_connections
        Milter::Logger.info("#{log_tag}[postfix] <#{threshold.inspect}>")
      elsif sendmail?
        threshold = detect_sendmail_threshold_n_connections
        Milter::Logger.info("#{log_tag}[sendmail] <#{threshold.inspect}>")
      else
        Milter::Logger.info("[breaker][detect][error][threshold] " +
                            "can't detect MTA")
      end
      threshold
    end

    def detect_postfix_threshold_n_connections
      process_limit = detect_postfix_process_limit
      if process_limit
        (process_limit * 0.75).to_i
      else
        0
      end
    end

    def detect_postfix_process_limit
      default_process_limit = nil
      master_cf = nil
      postfix_postconf.each_line do |line|
        case line
        when /\A(\w+)\s*=\s*(.+)\s*\z/
          key, value = $1, $2
          case key
          when "default_process_limit"
            default_process_limit = $2.to_i
          when "config_directory"
            master_cf = File.join(value, "master.cf")
          end
        end
      end
      process_limit = nil
      if master_cf
        process_limit ||= detect_postfix_process_limit_from_master_cf(master_cf)
      end
      process_limit ||= default_process_limit
      process_limit ||= 0
      process_limit
    end

    def detect_postfix_process_limit_from_master_cf(master_cf)
      return nil unless File.exist?(master_cf)
      File.open(master_cf) do |file|
        file.each_line do |line|
          next if /\A#/ =~ line
          service, type, _private, unpriv, chroot, wakeup, maxproc, command =
            line.split(/\s+/, 8)
          next if service != "smtp"
          return nil if maxproc == "-"
          return maxproc.to_i
        end
      end
      nil
    end

    def postfix_postconf
      postconf = detect_postfix_postconf
      result = `#{postconf} 2>&1`
      if $?.success?
        result
      else
        Milter::Logger.error("[breaker][detect][error][postfix][postconf] " +
                             "failed to run postconf: <#{result.strip}>")
        ""
      end
    end

    def detect_postfix_postconf
      prefix = detect_postfix_prefix
      postconf = nil
      if prefix
        Milter::Logger.debug("[breaker][detect][postfix][prefix] <#{prefix}>")
        postconf = "#{prefix}/sbin/postconf"
        postconf = nil unless File.executable?(postconf)
      end
      postconf ||= "postconf"
      Milter::Logger.debug("[breaker][detect][postfix][postconf] <#{postconf}>")
      postconf
    end

    def detect_postfix_prefix
      `ps ax -o command`.each_line do |line|
        case line
        when /\/lib(?:exec)?\/postfix\/master\b/
          return $PREMATCH
        end
      end
      nil
    end

    def detect_sendmail_threshold_n_connections
      0 # TODO
    end

    def postfix?
      `ps ax`.each_line do |line|
        return true if /master/ =~ line and /postfix/i =~ line
      end
      false
    end

    def sendmail?
      `ps ax`.each_line do |line|
        return true if /sendmail: sever/ =~ line
      end
      false
    end
  end
end
