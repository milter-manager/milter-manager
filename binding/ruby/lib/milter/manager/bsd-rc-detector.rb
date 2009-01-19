# Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
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
  class BSDRCDetector
    attr_reader :name, :variables
    def initialize(script_name, &connection_spec_detector)
      @script_name = script_name
      @connection_spec_detector = connection_spec_detector
      init_variables
    end

    def detect
      init_variables
      return unless rc_script_exist?

      parse_rc_script
      return if @name.nil?
      parse_rc_conf(rc_conf)
      parse_rc_conf(specific_rc_conf)
    end

    def rc_script_exist?
      File.exist?(rc_script)
    end

    def enabled?
      if /\AYES\z/i =~ (@variables["enable"] || "NO")
        true
      else
        false
      end
    end

    def apply(loader)
      return if @name.nil?
      connection_spec = guess_connection_spec
      return if connection_spec.nil?
      connection_spec = "unix:#{connection_spec}" if /\A\// =~ connection_spec
      loader.define_milter(@script_name) do |milter|
        milter.enabled = enabled?
        milter.command = rc_script
        milter.command_options = "start"
        milter.connection_spec = connection_spec
        yield(milter) if block_given?
      end
    end

    private
    def init_variables
      @name = nil
      @variables = {}
    end

    def parse_rc_script
      rc_script_content = File.read(rc_script)
      rc_script_content.each_line do |line|
        if /\Aname=(.+)/ =~ line
          @name = $1.sub(/\A"(.+)"\z/, '\1')
        end
      end
      return if @name.nil?

      rc_script_content.each_line do |line|
        if /\$\{#{Regexp.escape(@name)}_(.+?)(?::?-|=)(.*)\}/ =~ line
          variable_name, variable_value = $1, $2
          variable_value = normalize_variable_value(variable_value)
          @variables[variable_name] = variable_value
        end
      end
    end

    def parse_rc_conf(file)
      return unless File.exist?(file)
      content = File.read(file)
      content.each_line do |line|
        case line
        when /\A#{@name}_(.+)=(.+)/
          variable_name = $1
          variable_value = $2
          variable_value = normalize_variable_value(variable_value)
          @variables[variable_name] = variable_value
        end
      end
    end

    def normalize_variable_value(value)
      value.sub(/\A"(.*)"\z/, '\\1')
    end

    def guess_connection_spec
      spec = nil
      if @connection_spec_detector
        spec = @connection_spec_detector.call(self)
      end
      spec ||= @variables["socket"] || @variables["sockfile"]
      spec ||= @variables["connection_spec"]
      spec ||= extract_connection_spec_parameter_from_flags(@variables["flags"])
      spec
    end

    def extract_connection_spec_parameter_from_flags(flags)
      return nil if flags.nil?

      options = Shellwords.split(flags)
      p_option_index = options.index("-p")
      if p_option_index
        spec = options[p_option_index + 1]
        return spec unless shell_variable?(spec)
      else
        options.each do |option|
          if /\A-p/ =~ option
            spec = $POSTMATCH
            return spec unless shell_variable?(spec)
          end
        end
      end
      nil
    end

    def shell_variable?(string)
      /\A\$/ =~ string
    end

    def rc_script
      File.join(rc_d, @script_name)
    end

    def specific_rc_conf
      File.join(rc_conf_d, @name)
    end

    def rc_d
      "/usr/local/etc/rc.d"
    end

    def rc_conf
      "/etc/rc.conf"
    end

    def rc_conf_d
      "/etc/rc.conf.d"
    end
  end
end
