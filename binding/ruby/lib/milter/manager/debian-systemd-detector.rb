# Copyright (C) 2009-2023  Sutou Kouhei <kou@clear-code.com>
# Copyright (C) 2017  Kenji Okimoto <okimoto@clear-code.com>
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

require 'milter/manager/file-reader'
require 'milter/manager/systemd-detector'

module Milter::Manager
  class DebianSystemdDetector
    include SystemdDetector

    private
    def clamav_milter_conf
      extract_parameter_from_flags(@exec, "-c") ||
        etc_file("clamav", "clamav-milter.conf")
    end

    def milter_greylist_conf
      extract_parameter_from_flags(@exec, "-f") ||
        etc_file("milter-greylist", "greylist.conf")
    end

    def opendkim_conf
      extract_parameter_from_flags(@variables["OPTIONS"] || "", "-x") ||
        etc_file("opendkim.conf")
    end
  end
end
