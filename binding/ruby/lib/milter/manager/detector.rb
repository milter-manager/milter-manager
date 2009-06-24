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
  module Detector
    attr_reader :name, :variables

    def initialize(configuration, script_name, &connection_spec_detector)
      @configuration = configuration
      @script_name = script_name
      @connection_spec_detector = connection_spec_detector
      init_variables
    end

    def apply(loader)
      return if @name.nil?
      spec = guess_spec
      loader.define_milter(@script_name) do |milter|
        milter.enabled = enabled?
        milter.description = description
        milter.command = command
        milter.command_options = command_options
        if spec
          spec = "unix:#{spec}" if /\A\// =~ spec
          spec = spec.gsub(/\A(?:(unix|local):)+/, '\\1:')
          milter.connection_spec = spec
        end
        yield(milter) if block_given?
      end
    end

    def package_options
      @package_options ||= @configuration.parsed_package_options || {}
    end

    def command_options
      "start"
    end

    private
    def init_variables
      @name = nil
      @variables = {}
    end

    def set_variable(name, unnormalized_value)
      @variables[name] = normalize_variable_value(unnormalized_value)
    end

    def extract_parameter_from_flags(flags, option)
      return nil if flags.nil?

      parameter = nil

      options = Shellwords.split(flags)
      option_index = options.rindex(option)
      if option_index
        _parameter = options[option_index + 1]
        parameter = _parameter unless shell_variable?(_parameter)
      else
        options.each do |_option|
          if /\A#{Regexp.escape(option)}/ =~ _option
            _parameter = $POSTMATCH.sub(/\A=/, '')
            parameter = _parameter unless shell_variable?(_parameter)
          end
        end
      end

      parameter
    end

    def extract_variables(output, content, options={})
      if options[:accept_lower_case]
        variable_definition_re = /\b([A-Za-z\d_]+)=(".*?"|\S*)/
      else
        variable_definition_re = /\b([A-Z\d_]+)=(".*?"|\S*)/
      end

      content.each_line do |line|
        case line.sub(/#.*/, '')
        when variable_definition_re
          variable_name, variable_value = $1, $2
          variable_value = normalize_variable_value(variable_value)
          output[variable_name] = variable_value || output[variable_name]
        end
      end
    end

    def normalize_variable_value(value)
      value = value.sub(/\A"(.*)"\z/, '\\1')
      return nil if /\A\$\d+\z/ =~ value
      variable_expand_re = /\$(\{?)([a-zA-Z\d_]+)(\}?)/
      value.gsub(variable_expand_re) do |matched|
        left_brace, name, right_brace = $1, $2, $3
        if [left_brace, right_brace] == ["{", "}"] or
            [left_brace, right_brace] == ["", ""]
          expand_variable(name) || matched
        else
          matched
        end
      end
    end

    def expand_variable(name)
      if name == "name"
        @name
      else
        @variables[name]
      end
    end

    def extract_spec_parameter_from_flags(flags)
      extract_parameter_from_flags(flags, "-p")
    end

    def normalize_spec(spec)
      return nil if spec.nil?

      spec = spec.strip
      return nil if spec.empty?

      spec
    end

    def shell_variable?(string)
      /\A\$/ =~ string
    end
  end
end
