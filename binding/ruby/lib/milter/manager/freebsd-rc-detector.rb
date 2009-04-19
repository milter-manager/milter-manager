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

require 'milter/manager/rcng-detector'

module Milter::Manager
  class FreeBSDRCDetector
    include RCNGDetector

    def rcvar
      @rcvar || "#{@name}_enable"
    end

    def rcvar_value
      @rcvar_value || @variables[rcvar_variable_name] || "NO"
    end

    def detect_enma_connection_spec
      conf_file = @variables["cfgfile"]
      conf_file ||= extract_parameter_from_flags(command_args, "-c")
      return nil if conf_file.nil?
      Milter::Manager::EnmaSocketDetector.new(conf_file).detect
    end

    def enma?
      @script_name == "milter-enma" or @name == "milterenma"
    end

    private
    def parse_rc_conf_unknown_line(line)
      case line
      when /\Arcvar=`set_rcvar`/
        @rcvar = "#{@name}_enable"
      when /\A#{Regexp.escape(rcvar)}=(.+)/
        @rcvar_value = normalize_variable_value($1)
      end
    end

    def rcvar_variable_name
      rcvar.gsub(/\A#{Regexp.escape(@name)}_/, '')
    end

    def rc_d
      "/usr/local/etc/rc.d"
    end

    def guess_application_specific_spec
      spec = nil
      spec ||= detect_enma_connection_spec if enma?
      spec
    end
  end
end
