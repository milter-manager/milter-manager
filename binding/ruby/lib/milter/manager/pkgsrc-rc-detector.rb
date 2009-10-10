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
require 'milter/manager/enma-socket-detector'

module Milter::Manager
  class PkgsrcRCDetector
    include RCNGDetector

    def rcvar
      @rcvar || @name
    end

    def rcvar_value
      @rcvar_value || "NO"
    end

    def detect_enma_connection_spec
      conf_file = extract_parameter_from_flags(command_args, "-c")
      conf_file ||= File.join(package_prefix, "enma.conf")
      Milter::Manager::EnmaSocketDetector.new(conf_file).detect
    end

    def enma?
      @script_name == "enma" or @name == "enma"
    end

    def milter_greylist?
      @script_name == "milter-greylist" or @name == "miltergreylist"
    end

    private
    def parse_rc_conf_unknown_line(line)
      case line
      when /\Arcvar=(.+)/
        @rcvar = normalize_variable_value($1)
      when /\A#{Regexp.escape(rcvar)}=(.+)/
        @rcvar_value = normalize_variable_value($1)
      end
    end

    def rc_d
      package_options["rcddir"] || File.join(package_prefix, "rc.d")
    end

    def package_prefix
      package_options["prefix"] || "/usr/pkg/etc"
    end

    def guess_application_specific_spec
      spec = nil
      spec ||= detect_enma_connection_spec if enma?
      spec
    end

    def milter_greylist_conf
      extract_parameter_from_flags(command_args, "-f") ||
        File.join(package_prefix, "mail", "greylist.conf")
    end
  end
end
