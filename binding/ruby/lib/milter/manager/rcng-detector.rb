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

require 'milter/manager/detector'

module Milter::Manager
  module RCNGDetector
    include Detector

    def description
      nil
    end

    def command
      rc_script
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

    private
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

    def guess_spec
      spec = nil
      if @connection_spec_detector
        spec = normalize_spec(@connection_spec_detector.call(self))
      end
      spec ||= normalize_spec(@variables["socket"])
      spec ||= normalize_spec(@variables["sockfile"])
      spec ||= normalize_spec(@variables["connection_spec"])
      spec ||= extract_spec_parameter_from_flags(@variables["flags"])
      spec
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
