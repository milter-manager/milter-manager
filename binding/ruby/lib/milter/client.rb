# Copyright (C) 2009-2022  Sutou Kouhei <kou@clear-code.com>
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

require "milter/core"

begin
  require "milter_client.so"
rescue LoadError
  require "gobject-introspection"
  MilterClient = GObjectIntrospection.load("MilterClient")
  module Milter
    Client = MilterClient::Client
    class Client
      DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE =
        MilterClient::CLIENT_DEFAULT_SUSPEND_TIME_ON_UNACCEPTABLE
      DEFAULT_MAX_CONNECTIONS =
        MilterClient::CLIENT_DEFAULT_MAX_CONNECTIONS

      Milter.define_backward_compatible_enum_constants(
        self,
        MilterClient::ClientEventLoopBackend,
        "EVENT_LOOP_BACKEND")

      alias_method :start_syslog_raw, :start_syslog
      def start_syslog(identify, facility=nil)
        set_syslog_identify(identify)
        set_syslog_facility(facility) if facility
        start_syslog_raw
      end

      alias_method :set_unix_socket_mode_raw, :set_unix_socket_mode
      def set_unix_socket_mode(mode)
        case mode
        when nil
          mode = 0
        when String
          original_mode = mode
          success, mode, error_message = Milter.utils_parse_file_mode(mode)
          raise ArgumentError, error_message unless success
        end
        set_unix_socket_mode_raw(mode)
      end
      alias_method :unix_socket_mode=, :set_unix_socket_mode
    end

    ClientContext = MilterClient::ClientContext
    class ClientContext
      Milter.define_backward_compatible_enum_constants(
        self,
        MilterClient::ClientContextState,
        "STATE")
    end

    ClientContextState = MilterClient::ClientContextState

    ClientEventLoopBackend = MilterClient::ClientEventLoopBackend
  end
end

require "milter/client/session"
require "milter/client/fallback-session"
require "milter/client/session-context"
require "milter/client/configuration"
require "milter/client/context-state"
require "milter/client/command-line"
require "milter/client/composite-session"
require "milter/client/envelope-address"
require "milter/client/mail-transaction-shelf"

module Milter
  class Client
    def fallback_status
      @fallback_status ||= :accept
    end

    def fallback_status=(status)
      @fallback_status = status
    end

    # just for backward compatibility.
    alias_method :status_on_error, :fallback_status
    alias_method :status_on_error=, :fallback_status=

    def register(session_class, *new_arguments)
      signal_connect("connection-established") do |_client, context|
        begin
          setup_session(context, session_class, new_arguments)
        rescue Exception
          Milter::Logger.error($!)
          setup_session(context, ClientFallbackSession, [fallback_status])
        end
      end
    end

    def on_error
      signal_connect("error") do |_client, error|
        Milter::Callback.guard do
          yield(_client, error)
        end
      end
    end

    def on_maintain
      signal_connect("maintain") do |_client|
        Milter::Callback.guard do
          yield(_client)
        end
      end
    end

    def on_event_loop_created
      signal_connect("event-loop-created") do |_client, event_loop|
        Milter::Callback.guard do
          yield(_client, event_loop)
        end
      end
    end

    def reload
      reload_callbacks.each do |callback|
        Milter::Callback.guard do
          callback.call(self)
        end
      end
    end

    def on_reload(&callback)
      raise ArgumentError, "callback block on reload is missing" if callback.nil?
      reload_callbacks << callback
    end

    private
    def setup_session(context, session_class, session_new_arguments)
      context.use_bytes = true
      session_context = ClientSessionContext.new(context)
      session = session_class.new(session_context, *session_new_arguments)

      [:negotiate, :connect, :helo, :envelope_from, :envelope_recipient,
       :data, :unknown, :header, :end_of_header, :body, :end_of_message,
       :finished].each do |event|
        next unless session.respond_to?(event)
        if event == :body
          signal = "body-bytes"
        elsif event == :end_of_message
          signal = "end_of_message_bytes"
        else
          signal = event
        end
        context.signal_connect(signal) do |_context, *args|
          begin
            case event
            when :connect
              host, address, address_size = args
              address = SocketAddress.resolve(address, address_size)
              session.send(event, host, address)
            when :body
              body, = args
              session.send(event, body.to_s)
            when :end_of_message
              session.send(event)
            else
              session.send(event, *args)
            end
          rescue Exception
            Milter::Logger.error($!)
            session_context.status = fallback_status
          end
          status = session_context.status
          session_context.clear
          status
        end
      end

      context.signal_connect(:abort) do |_context, *args|
        if session.respond_to?(:abort)
          begin
            session.abort(*args)
          rescue Exception
            Milter::Logger.error($!)
            session_context.status = fallback_status
          end
        end
        begin
          session.reset
        rescue Exception
          Milter::Logger.error($!)
          session_context.status = fallback_status
        end
        status = session_context.status
        session_context.clear
        status
      end

      @sessions ||= {}
      @sessions[session] = true
      context.signal_connect(:finished) do
        @sessions.delete(session)
      end
    end

    def reload_callbacks
      @reload_callbacks ||= []
    end
  end
end
