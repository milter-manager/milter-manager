# Copyright (C) 2009-2010  Kouhei Sutou <kou@clear-code.com>
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
  module FreeBSDRCBaseDetector
    include RCNGDetector

    def rcvar
      @rcvar || "#{rcvar_prefix}_enable"
    end

    def rcvar_value
      @rcvar_value ||
        @variables[rcvar.gsub(/\A#{Regexp.escape(rcvar_prefix)}_/, '')] ||
        default_rcvar_value
    end

    def detect_enma_connection_spec
      Milter::Manager::EnmaSocketDetector.new(enma_conf).detect
    end

    def enma?
      @script_name == "milter-enma" or @name == "milterenma"
    end

    def detect_clamav_milter_connection_spec
      clamav_milter_config_parser.milter_socket
    end

    def clamav_milter?
      @script_name == "clamav-milter" or @name == "clamav_milter"
    end

    def milter_greylist?
      @script_name == "milter-greylist" or @name == "miltergreylist"
    end

    def opendkim?
      @script_name == "milter-opendkim" or @name == "milteropendkim"
    end

    private
    def enma_conf
      @variables["cfgfile"] ||
        extract_parameter_from_flags(command_args, "-c") ||
        "/usr/local/etc/enma.conf"
    end

    def clamav_milter_conf
      @other_variables["conf_file"] ||
        extract_parameter_from_flags(command_args, "-c") ||
        "/usr/local/etc/clamav-milter.conf"
    end

    def milter_greylist_conf
      @variables["cfgfile"] ||
        extract_parameter_from_flags(@variables["flags"], "-f") ||
        "/usr/local/etc/mail/greylist.conf"
    end

    def opendkim_conf
      @variables["cfgfile"] ||
        extract_parameter_from_flags(command_args, "-C") ||
        "/usr/local/etc/opendkim.conf"
    end

    def parse_rc_conf_unknown_line(line)
      case line
      when /\Arcvar=`set_rcvar`/
        @rcvar = "#{rcvar_prefix}_enable"
      when /\A#{Regexp.escape(rcvar)}=(.+)/
        @rcvar_value = normalize_variable_value($1)
      end
    end

    def guess_application_specific_spec
      spec = nil
      spec ||= detect_enma_connection_spec if enma?
      spec ||= detect_clamav_milter_connection_spec if clamav_milter?
      spec ||= detect_opendkim_connection_spec if opendkim?
      spec
    end
  end

  class FreeBSDRCDetector
    include FreeBSDRCBaseDetector

    def profiles
      _profiles = @variables["profiles"]
      if _profiles.nil?
        nil
      else
        _profiles.split
      end
    end

    def apply(loader)
      _profiles = profiles
      if opendkim? and _profiles and !_profiles.empty?
        _profiles.each do |profile|
          detector = FreeBSDRCProfileDetector.new(@configuration,
                                                  @script_name,
                                                  profile,
                                                  self,
                                                  &@connection_spec_detector)
          detector.detect
          detector.apply(loader)
        end
      else
        super
      end
    end

    private
    def default_rcvar_value
      "NO"
    end

    def rc_d
      "/usr/local/etc/rc.d"
    end
  end

  class FreeBSDRCProfileDetector
    include FreeBSDRCBaseDetector

    def initialize(configuration, script_name, profile_name, base_detector,
                   &connection_spec_detector)
      super(configuration, script_name, &connection_spec_detector)
      @profile_name = profile_name
      @base_detector = base_detector
    end

    protected
    def rcvar_prefix
      "#{@name}_#{@profile_name}"
    end

    private
    def init_variables
      super
      @base_variables = {}
    end

    def need_apply?
      super and not @profile_name.nil?
    end

    def milter_name
      [super, @profile_name].join("-")
    end

    def command_options
      super + [@profile_name]
    end

    def rc_d
      @base_detector.rc_d
    end

    def rc_conf
      @base_detector.rc_conf
    end

    def rc_conf_d
      @base_detector.rc_conf_d
    end

    def candidate_service_commands
      @base_detector.candidate_service_commands
    end

    def detect_opendkim_connection_spec
      super || @base_variables["socket"]
    end

    def parse_rc_conf_unknown_line(line)
      case line
      when /\A#{Regexp.escape(@base_detector.rcvar_prefix)}_(.+)=(.+)/
        variable_name = $1
        variable_value = $2
        variable_value = normalize_variable_value(variable_value)
        @base_variables[variable_name] = variable_value
      else
        super
      end
    end

    def default_rcvar_value
      @base_variables["enable"] || super
    end
  end
end
