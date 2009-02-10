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
  class ChildContext
    def initialize(child, children, quitted=false)
      @child = child
      @children = children
      @quitted = quitted
    end

    def name
      @child.name
    end

    def [](name)
      (@macros ||= @child.available_macros || {})[normalize_macro_name(name)]
    end

    def []=(name, value)
      @child.set_macro(@child.macro_context, normalize_macro_name(name), value)
      @macros = nil
      value
    end

    def reject?
      @child.status == Milter::STATUS_REJECT
    end

    def temporary_failure?
      @child.status == Milter::STATUS_TEMPORARY_FAILURE
    end

    def accept?
      @child.status == Milter::STATUS_ACCEPT
    end

    def discard?
      @child.status == Milter::STATUS_DISCARD
    end

    def quitted?
      @quitted
    end

    def children
      @child_contexts ||= create_child_contexts
    end

    def postfix?
      if /\bPostfix\b/i =~ (self["v"] || '')
        true
      else
        false
      end
    end

    def authenticated?
      if self["auth_type"] || self["auth_authen"]
        true
      else
        false
      end
    end

    private
    def create_child_contexts
      contexts = {}
      @children.children.each do |child|
        contexts[child.name] = self.class.new(child, @children, false)
      end
      @children.quitted_children.each do |child|
        contexts[child.name] = self.class.new(child, @children, true)
      end
      contexts
    end

    def normalize_macro_name(name)
      name.sub(/\A\{(.+)\}\z/, '\1')
    end
  end
end
