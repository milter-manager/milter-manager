# Copyright (C) 2009-2010  Kouhei Sutou <kou@clear-code.com>
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

require 'milter/manager/detector'

module Milter::Manager
  module RCNGDetector
    include Detector

    attr_reader :command_args
    def initialize(*args, &block)
      super
      @rcvar = nil
      @rcvar_value = nil
      @command_args = nil
    end

    def description
      nil
    end

    def run_command
      rc_script
    end

    def detect
      init_variables
      return unless rc_script_readable?

      parse_rc_script
      return if @name.nil?
      parse_rc_conf(rc_conf)
      parse_rc_conf(rc_conf_local)
      parse_rc_conf(specific_rc_conf)
    end

    def rc_script_readable?
      File.readable?(rc_script)
    end
    # For backward compatibility. TODO: warning message.
    alias_method :rc_script_exist?, :rc_script_readable?

    def enabled?
      if /\AYES\z/i =~ rcvar_value
        true
      else
        false
      end
    end

    protected
    def rcvar_prefix
      @name
    end

    private
    def init_variables
      super
      @other_variables = {}
    end

    def parse_rc_script
      rc_script_content = File.read(rc_script)
      rc_script_content.each_line do |line|
        if /\Aname=(.+)/ =~ line
          @name = normalize_variable_value($1)
        end
      end
      return if @name.nil?

      extract_variables(@other_variables, rc_script_content,
                        :accept_lower_case => true)

      before_load_rc_conf = true
      _rcvar_prefix = Regexp.escape(rcvar_prefix)
      rc_script_content.each_line do |line|
        case line
        when /\A\s*load_rc_conf /
          before_load_rc_conf = false
        when /\$\{#{_rcvar_prefix}_(.+?)(?::?-|=)(.*)\}/
          set_variable($1, $2)
        when /\A#{_rcvar_prefix}_(.+?)=(.*)/
          set_variable($1, $2) if before_load_rc_conf
        when /\Acommand_args=(.+)/
          @command_args = normalize_variable_value($1)
        end
      end
    end

    def parse_rc_conf(file)
      return unless File.readable?(file)
      File.open(file) do |conf|
        _rcvar_prefix = Regexp.escape(rcvar_prefix)
        conf.each_line do |line|
          case line
          when /\A#{_rcvar_prefix}_(.+)=(.+)/
            variable_name = $1
            variable_value = $2
            variable_value = normalize_variable_value(variable_value)
            @variables[variable_name] = variable_value
          else
            parse_rc_conf_unknown_line(line)
          end
        end
      end
    end

    def parse_rc_conf_unknown_line(line)
    end

    def expand_variable(name)
      @other_variables[name] || super(name)
    end

    def guess_spec
      spec = nil
      spec ||= guess_application_specific_spec
      spec ||= normalize_spec(@variables["socket"])
      spec ||= normalize_spec(@variables["sockfile"])
      spec ||= normalize_spec(@variables["connection_spec"])
      spec ||= extract_spec_parameter_from_flags(@variables["flags"])
      spec ||= extract_spec_parameter_from_flags(@command_args)
      if @connection_spec_detector
        spec = normalize_spec(@connection_spec_detector.call(self, spec)) || spec
      end
      spec
    end

    def rc_script
      File.join(rc_d, @script_name)
    end

    def specific_rc_conf
      File.join(rc_conf_d, @name)
    end

    def rc_conf
      "/etc/rc.conf"
    end

    def rc_conf_local
      "/etc/rc.conf.local"
    end

    def rc_conf_d
      "/etc/rc.conf.d"
    end
  end
end
