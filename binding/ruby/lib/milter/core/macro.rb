# Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

module Milter
  class ProtocolAgent
    def set_macros(context, macros)
      macros.each do |name, value|
        set_macro(context, name, value)
      end
    end
  end

  module MacroNameNormalizer
    def normalize_macro_name(name)
      name.sub(/\A\{(.+)\}\z/, '\1')
    end
  end

  module MacroPredicates
    def authenticated?
      (self["auth_type"] or self["auth_authen"]) ? true : false
    end

    def postfix?
      (/\bPostfix\b/i =~ (self["v"] || '')) ? true : false
    end
  end
end
