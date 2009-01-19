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
  class DebianInitDetector
    attr_reader :name, :variables, :info
    def initialize(script_name, &connection_spec_detector)
      @script_name = script_name
      @connection_spec_detector = connection_spec_detector
      init_variables
    end

    def detect
      init_variables
      return unless init_script_exist?

      parse_init_script
      return if @name.nil?
      parse_default_conf(default_conf)
    end

    def init_script_exist?
      File.exist?(init_script)
    end

    def description
      @info["Description"] || @variables["DESC"]
    end

    def enabled?
      @variables["ENABLED"] != "0"
    end

    def apply(loader)
      return if @name.nil?
      connection_spec = guess_connection_spec
      return if connection_spec.nil?
      connection_spec = "unix:#{connection_spec}" if /\A\// =~ connection_spec
      connection_spec = connection_spec.gsub(/\A(?:(unix|local):)+/, '\\1:')
      loader.define_milter(@script_name) do |milter|
        milter.enabled = enabled?
        milter.description = description
        milter.command = init_script
        milter.command_options = "start"
        milter.connection_spec = connection_spec
        yield(milter) if block_given?
      end
    end

    private
    def init_variables
      @name = nil
      @info = {}
      @variables = {}
    end

    VARIABLE_DEFINITION_RE = /\b([A-Z\d_]+)=(".*?"|\S*)/
    def parse_init_script
      content = File.open(init_script).read
      info = content.scan(/^### BEGIN INIT INFO\n(.+)\n### END INIT INFO$/m)
      parse_init_info(info[0][0]) if info[0]
      extract_variables(content)

      @name = @variables["NAME"] || @info["Provides"] || @script_name
    end

    def parse_init_info(info)
      info = info.gsub(/^# /, '').split(/([a-zA-Z\-]+):\s*/m)[1..-1]
      0.step(info.size - 1, 2) do |i|
        key, value = info[i], (info[i + 1] || '')
        @info[key] = value.gsub(/\s*\n\s*/, ' ').strip
      end
    end

    def parse_default_conf(file)
      return unless File.exist?(file)
      File.open(file) do |input|
        extract_variables(input)
      end
    end

    def extract_variables(input)
      input.each_line do |line|
        case line.sub(/#.*/, '')
        when VARIABLE_DEFINITION_RE
          variable_name, variable_value = $1, $2
          variable_value = normalize_variable_value(variable_value)
          @variables[variable_name] = variable_value
        end
      end
    end

    def normalize_variable_value(value)
      value = value.sub(/\A"(.*)"\z/, '\\1')
      variable_expand_re = /\$(\{?)([a-zA-Z\d_]+)(\}?)/
      value = value.gsub(variable_expand_re) do |matched|
        left_brace, name, right_brace = $1, $2, $3
        if [left_brace, right_brace] == ["{", "}"] or
            [left_brace, right_brace] == ["", ""]
          @variables[name] || matched
        else
          matched
        end
      end
    end

    def guess_connection_spec
      spec = nil
      if @connection_spec_detector
        spec = @connection_spec_detector.call(self)
      end
      spec ||= @variables["SOCKET"] || @variables["SOCKFILE"]
      spec ||= @variables["CONNECTION_SPEC"]
      spec ||= extract_connection_spec_parameter_from_flags(@variables["OPTARGS"])
      spec ||= extract_connection_spec_parameter_from_flags(@variables["DAEMON_ARGS"])
      spec ||= extract_connection_spec_parameter_from_flags(@variables["PARAMS"])
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
            return spec unless shell_variable?(_spec)
          end
        end
      end
      nil
    end

    def shell_variable?(string)
      /\A\$/ =~ string
    end

    def init_script
      File.join(init_d, @script_name)
    end

    def default_conf
      File.join(default_dir, @script_name)
    end

    def init_d
      "/etc/init.d"
    end

    def default_dir
      "/etc/default"
    end
  end
end
