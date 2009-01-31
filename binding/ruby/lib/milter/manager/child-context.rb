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
    def initialize(child, children)
      @child = child
      @children = children
    end

    def name
      @child.name
    end

    def [](name)
      (@child.available_macros || {})[name]
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

    def processing?
      [Milter::STATUS_CONTINUE,
       Milter::STATUS_NOT_CHANGE].include?(@child.status)
    end

    def children
      @child_contexts ||= create_child_contexts
    end

    private
    def create_child_contexts
      contexts = {}
      @children.each do |child|
        contexts[child.name] = self.class.new(child, @children)
      end
      contexts
    end
  end
end
