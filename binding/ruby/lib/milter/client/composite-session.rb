# Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
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
  class Client
    class CompositeSession
      def initialize(context, session_classes, *arguments)
        @context = context
        @sessions = session_classes.collect do |session_class|
          session_class.new(@context, *arguments)
        end
      end

      def negotiate(option, macros_requests)
        apply_sessions(:negotiate, :reject, option, macros_requests)
      end

      def connect(host, address)
        apply_sessions(:connect, :continue, hosts, address)
      end

      def helo(fqdn)
        apply_sessions(:helo, :continue, fqdn)
      end

      def envelope_from(from)
        apply_sessions(:envelope_from, :continue, from)
      end

      def envelope_recipient(to)
        apply_sessions(:envelope_recipient, :continue, to)
      end

      def data
        apply_sessions(:data, :continue)
      end

      def header(name, value)
        apply_sessions(:header, :continue, name, value)
      end

      def end_of_header
        apply_sessions(:end_of_header, :continue)
      end

      def body(chunk)
        apply_sessions(:body, :continue, chunk)
      end

      def end_of_message
        apply_sessions(:end_of_message, :continue)
      end

      private
      def apply_sessions(method_name, default_status, *arguments)
        status = default_status
        @sessions.each do |session|
          next unless session.respond_to?(method_name)
          session.send(method_name, *arguments)
          status = resolve_status(status, @context.status)
        end
        @context.status = status
      end

      def resolve_status(current_status, new_status)
        current_status = ensure_status(current_status)
        new_status = ensure_status(new_status)
        if (current_status <=> new_status) < 0
          new_status
        else
          current_status
        end
      end

      def ensure_status(status)
        return status if status.is_a?(Milter::Status)
        Milter::Status.const_get(status.to_s.upcase)
      end
    end
  end
end

