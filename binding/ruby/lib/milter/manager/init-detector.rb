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

require 'milter/manager/detector'

module Milter::Manager
  module InitDetector
    include Detector

    attr_reader :info
    def detect
      return unless init_script_readable?

      parse_init_script
      return if @name.nil?
      parse_custom_conf
    end

    def run_command
      init_script
    end

    def init_script_readable?
      File.readable?(init_script)
    end
    # For backward compatibility. TODO: warning message.
    alias_method :init_script_exist?, :init_script_readable?

    private
    def init_variables
      super
      @info = {}
    end

    INIT_INFO_BLOCK_RE = /^### BEGIN INIT INFO\n(.+)\n### END INIT INFO$/m
    def extract_meta_data_blocks(content)
      first_comment_block = content.split(/^[^#]/)[0] || ''
      before, init_info_content, after =
        first_comment_block.split(INIT_INFO_BLOCK_RE, 2)
      [before, init_info_content, after]
    end

    def parse_init_info(info)
      info = info.gsub(/^# /, '').split(/([a-zA-Z\-]+):\s*/m)[1..-1]
      0.step(info.size - 1, 2) do |i|
        key, value = info[i], (info[i + 1] || '')
        @info[key] = value.gsub(/\s*\n\s*/, ' ').strip
      end
    end

    def init_script
      File.join(init_d, @script_name)
    end

    def init_d
      File.join(init_base_dir, "init.d")
    end

    def init_base_dir
      etc_dir
    end

    def etc_dir
      File.join("", "etc")
    end

    def etc_file(*paths)
      File.join(etc_dir, *paths)
    end
  end
end
