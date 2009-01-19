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
  class RedHatInitDetector
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
      parse_sysconfig_conf(sysconfig_conf)
    end

    def init_script_exist?
      File.exist?(init_script)
    end

    def description
      @info["description"] || @variables["DESC"]
    end

    def enabled?
      not guess_connection_spec.nil?
    end

    def apply(loader)
      return if @name.nil?
      spec = guess_connection_spec
      loader.define_milter(@script_name) do |milter|
        milter.enabled = enabled?
        milter.description = description
        milter.command = init_script
        milter.command_options = "start"
        if spec
          spec = "unix:#{spec}" if /\A\// =~ spec
          spec = spec.gsub(/\A(?:(unix|local):)+/, '\\1:')
          milter.connection_spec = spec
        end
        yield(milter) if block_given?
      end
    end

    private
    def init_variables
      @name = nil
      @info = {}
      @variables = {}
    end

    VARIABLE_DEFINITION_RE = /\b([A-Za-z\d_]+)=(".*?"|\S*)/
    INIT_INFO_BLOCK_RE = /^### BEGIN INIT INFO\n(.+)\n### END INIT INFO$/m
    def parse_init_script
      content = File.open(init_script).read

      first_comment_block = content.split(/^[^#]/)[0] || ''
      before, init_info, after =
        first_comment_block.split(INIT_INFO_BLOCK_RE, 2)
      parse_tag_block(before)
      parse_init_info(init_info) if init_info
      parse_tag_block(after) if after

      extract_variables(content)

      @name = @script_name
    end

    def parse_tag_block(block)
      description_continue = false
      block.gsub(/^# /, '').each_line do |line|
        if description_continue
          description_continue = false
          value = line.strip
          if /\\\z/ =~ value
            value = $PREMATCH
            description_continue = true
          end
          @info["description"] << value
          next
        end

        case line
        when /\A(chkconfig|processname|config|pidfile|probe):\s*/
          @info[$1] = $POSTMATCH.strip
        when /\Adescription:\s*/
          value = $POSTMATCH.strip
          if /\\\z/ =~ value
            value = $PREMATCH
            description_continue = true
          end
          @info["description"] = value
        end
      end
    end

    def parse_init_info(info)
      info = info.gsub(/^# /, '').split(/([a-zA-Z\-]+):\s*/m)[1..-1]
      0.step(info.size - 1, 2) do |i|
        key, value = info[i], (info[i + 1] || '')
        @info[key] = value.gsub(/\s*\n\s*/, ' ').strip
      end
    end

    def parse_sysconfig_conf(file)
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
        spec = normalize_spec(@connection_spec_detector.call(self))
      end
      spec ||= normalize_spec(@variables["SOCKET"])
      spec ||= normalize_spec(@variables["SOCKFILE"])
      spec ||= normalize_spec(@variables["SOCKET_ADDRESS"])
      spec ||= normalize_spec(@variables["MILTER_SOCKET"])
      spec ||= normalize_spec(@variables["CONNECTION_SPEC"])
      spec ||= extract_connection_spec_parameter_from_flags(@variables["OPTIONS"])
      spec ||= extract_connection_spec_parameter_from_flags(@variables["DAEMON_ARGS"])
      spec ||= extract_connection_spec_parameter_from_flags(@variables["PARAMS"])
      spec
    end

    def normalize_spec(spec)
      return nil if spec.nil?

      spec = spec.strip
      return nil if spec.empty?

      spec
    end

    def extract_connection_spec_parameter_from_flags(flags)
      return nil if flags.nil?

      spec = nil

      options = Shellwords.split(flags)
      p_option_index = options.rindex("-p")
      if p_option_index
        _spec = options[p_option_index + 1]
        spec = _spec unless shell_variable?(_spec)
      else
        options.each do |option|
          if /\A-p/ =~ option
            _spec = $POSTMATCH
            spec = _spec unless shell_variable?(_spec)
          end
        end
      end

      spec
    end

    def shell_variable?(string)
      /\A\$/ =~ string
    end

    def init_script
      File.join(init_d, @script_name)
    end

    def sysconfig_conf
      File.join(sysconfig_dir, @script_name)
    end

    def init_d
      "/etc/init.d"
    end

    def sysconfig_dir
      "/etc/sysconfig"
    end
  end
end
