# Copyright (C) 2016  Kouhei Sutou <kou@clear-code.com>
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

require "milter/manager/file-reader"
require "milter/manager/systemd-detector"

module Milter::Manager
  class RedHatSystemdDetector
    include SystemdDetector

    private
    def clamav_milter_conf
      extract_parameter_from_flags(@exec, "-c") ||
        etc_file("mail", "clamav-milter.conf")
    end

    def milter_greylist_conf
      extract_parameter_from_flags(@exec, "-f") ||
        etc_file("mail", "greylist.conf")
    end

    def opendkim_conf
      extract_parameter_from_flags(@variables["OPTIONS"] || "", "-x") ||
        etc_file("opendkim.conf")
    end

    def rmilter_conf
      extract_parameter_from_flags(@exec, "-c") ||
        etc_file("rmilter", "rmilter.conf")
    end

    def system_dir
      File.join("", "usr", "lib", "systemd", "system")
    end
  end
end
