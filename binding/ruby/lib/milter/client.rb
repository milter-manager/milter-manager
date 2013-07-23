# Copyright (C) 2009-2013  Kouhei Sutou <kou@clear-code.com>
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

require "milter_client.so"
require "milter/client/configuration"
require "milter/client/context-state"
require "milter/client/command-line"
require "milter/client/composite-session"

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
        setup_session(context, session_class, new_arguments)
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
      session_context = ClientSessionContext.new(context)
      session = session_class.new(session_context, *session_new_arguments)

      [:negotiate, :connect, :helo, :envelope_from, :envelope_recipient,
       :data, :unknown, :header, :end_of_header, :body, :end_of_message,
       :finished].each do |event|
        next unless session.respond_to?(event)
        context.signal_connect(event) do |_context, *args|
          begin
            if event == :end_of_message
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
    end

    def reload_callbacks
      @reload_callbacks ||= []
    end
  end

  class ClientSession
    def initialize(context)
      @context = context
      reset
    end

    def negotiate(option, macros_requests)
      [:connect, :helo, :envelope_from, :envelope_recipient,
       :body, :unknown, [:header, :headers], :end_of_header, :data,
      ].each do |method_name, step_name|
        next unless respond_to?(method_name)
        step_name ||= method_name
        step = Milter::StepFlags.const_get("NO_#{step_name.to_s.upcase}")
        option.remove_step(step)
      end
      unless need_header_value_with_leading_space?
        option.remove_step(Milter::StepFlags::HEADER_VALUE_WITH_LEADING_SPACE)
      end
      option.remove_step(Milter::StepFlags::NO_REPLY_MASK)
      continue
    end

    def reset
    end

    private
    def need_header_value_with_leading_space?
      false
    end

    def accept
      @context.status = :accept
      reset
    end

    def discard
      @context.status = :discard
      reset
    end

    def continue
      @context.status = :continue
    end

    def delay_response
      @context.status = :progress
    end

    def progress
      @context.progress
    end

    def reject(options={})
      code = options[:code]
      extended_code = options[:extended_code]
      reason = options[:reason]
      if code or extended_code or reason
	# TODO: validate parameters.
        code ||= 550
        extended_code ||= "5.7.1"
        reason ||= "Command rejected"
        @context.set_reply(code, extended_code, reason)
      else
        @context.status = :reject
      end
      reset unless @context.state.envelope_recipient?
    end

    def temporary_failure(options={})
      code = options[:code]
      extended_code = options[:extended_code]
      reason = options[:reason]
      if code or extended_code or reason
	# TODO: validate parameters.
        code ||= 451
        extended_code ||= "4.7.1"
        reason ||= "Service unavailable - try again later"
        @context.set_reply(code, extended_code, reason)
      else
        @context.status = :temporary_failure
      end
      reset unless @context.state.envelope_recipient?
    end

    def quarantine(reason)
      if @context.quarantine(reason)
        accept
        true
      else
        false
      end
    end

    def add_header(name, value)
      @context.add_header(name, value)
    end

    def insert_header(index, name, value)
      @context.insert_header(index, name, value)
    end

    def change_header(name, index, value)
      @context.change_header(name, index, value)
    end

    def delete_header(name, index)
      @context.delete_header(name, index)
    end

    def replace_body(chunk)
      @context.replace_body(chunk)
    end

    def change_from(from)
      @context.change_from(from)
    end

    def add_recipient(recipient, parameters=nil)
      @context.add_recipient(recipient, parameters)
    end

    def delete_recipient(recipient)
      @context.delete_recipient(recipient)
    end

    def postfix?
      @context.postfix?
    end

    def authenticated?
      @context.authenticated?
    end

    def watch_io(channel, condition, options=nil, &block)
      @context.event_loop.watch_io(interval, options, &block)
    end

    def watch_child(pid, options=nil, &block)
      @context.event_loop.watch_child(pid, options, &block)
    end

    def add_idle(options=nil, &block)
      @context.event_loop.add_idle(options, &block)
    end

    def add_timeout(interval, options=nil, &block)
      @context.event_loop.add_timeout(interval, options, &block)
    end

    def remove_event(tag)
      @context.event_loop.remove(tag)
    end

    def [](name)
      @context[name]
    end

    def worker_id
      @context.client.worker_id
    end
  end

  class ClientSessionContext
    include MacroPredicates

    def initialize(context)
      @context = context
      clear
    end

    def status=(value)
      case value
      when String, Symbol
        unless Milter::Status.const_defined?(value.to_s.upcase)
          raise ArgumentError, "unknown status: <#{value.inspect}>"
        end
      when Milter::Status, nil
      else
        message =
          "should be one of [String, Symbol, Milter::Status, nil]: " +
          "<#{value.inspect}>"
        raise ArgumentError, message
      end
      @status = value
    end

    def status
      status = @status || Milter::Status::DEFAULT
      if status.is_a?(String) or status.is_a?(Symbol)
        status = Milter::Status.const_get(status.to_s.upcase)
      end
      status
    end

    def set_reply(code, extended_code, reason)
      @context.set_reply(code, extended_code, reason)
      if 400 <= code and code < 500
        self.status = :temporary_failure
      elsif 500 <= code and code < 600
        self.status = :reject
      end
    end

    def [](name)
      (@macros ||= @context.available_macros || {})[name]
    end

    def clear
      @macros = nil
      @status = nil
    end

    def method_missing(name, *args, &block)
      if @context.respond_to?(name)
        @context.send(name, *args, &block)
      else
        super
      end
    end
  end
end
