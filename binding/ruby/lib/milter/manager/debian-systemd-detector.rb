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

    def rmilter_conf
      extract_parameter_from_flags(@exec, "-c") ||
        etc_file("rmilter.conf")
    end
  end
end
