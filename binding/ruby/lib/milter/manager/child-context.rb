# Copyright (C) 2009-2011  Kouhei Sutou <kou@clear-code.com>
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
    include Milter::MacroNameNormalizer
    include Milter::MacroPredicates

    def initialize(child, children, client_context)
      @child = child
      @children = children
      @client_context = client_context
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
      @child.quitted?
    end

    def children
      @child_contexts ||= create_child_contexts
    end

    def n_processing_sessions
      @client_context.n_processing_sessions
    end

    private
    def create_child_contexts
      contexts = {}
      @children.children.each do |child|
        contexts[child.name] = self.class.new(child, @children, @client_context)
      end
      contexts
    end
  end
end
