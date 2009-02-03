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

    def initialize(script_name, &connection_spec_detector)
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
      @variables = {}
    end

    def set_variable(name, unnormalized_value)
      @variables[name] = normalize_variable_value(unnormalized_value)
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
          output[variable_name] = normalize_variable_value(variable_value)
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
