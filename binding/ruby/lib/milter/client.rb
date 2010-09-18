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

module Milter
  class Client
    def status_on_error
      @status_on_error ||= :accept
    end

    def status_on_error=(status)
      @status_on_error = status
    end

    def register(session_class, *new_arguments)
      signal_connect("connection-established") do |_client, context|
        setup_session(context, session_class, new_arguments)
      end
    end

    def on_error
      signal_connect("error") do |_client, error|
        yield(_client, error)
      end
    end

    def on_maintain
      signal_connect("maintain") do |_client|
        yield(_client)
      end
    end

    private
    def setup_session(context, session_class, session_new_arguments)
      session_context = ClientSessionContext.new(context)
      session = session_class.new(session_context, *session_new_arguments)

      [:negotiate, :connect, :helo, :envelope_from, :envelope_recipient,
       :data, :unknown, :header, :end_of_header, :body, :end_of_message,
       :abort, :finished].each do |event|
        if session.respond_to?(event)
          context.signal_connect(event) do |_context, *args|
            begin
              if event == :end_of_message
                session.send(event)
              else
                session.send(event, *args)
              end
            rescue Exception
              Milter::Logger.error($!)
              session_context.status = status_on_error
            end
            status = session_context.status
            session_context.status = :continue
            status
          end
        end
      end
    end
  end

  class ClientSession
    def initialize(context)
      @context = context
    end

    def negotiate(option, macros_requests)
      [:connect, :helo, :envelope_from, :envelope_recipient,
       :body, :unknown, [:header, :headers], :end_of_header, :data,
      ].each do |method_name, step_name|
        if respond_to?(method_name)
          step_name ||= method_name
          step = Milter::StepFlags.const_get("NO_#{step_name.to_s.upcase}")
          option.remove_step(step)
        end
      end
      unless need_header_value_with_leading_space?
        option.remove_step(Milter::StepFlags::HEADER_VALUE_WITH_LEADING_SPACE)
      end
      option.remove_step(Milter::StepFlags::NO_REPLY_MASK)
      continue
    end

    private
    def need_header_value_with_leading_space?
      false
    end

    def accept
      @context.status = :accept
    end

    def continue
      @context.status = :continue
    end

    def progress
      @context.status = :progress
    end

    def discard
      @context.status = :discard
    end

    def reject(options={})
      code = options[:code]
      extended_code = options[:extended_code]
      reason = options[:reason]
      if code
	# TODO: validate parameters.
        @context.set_reply(code, extended_code, reason)
      else
        @context.status = :reject
      end
    end

    def temporary_failure(options={})
      code = options[:code]
      extended_code = options[:extended_code]
      reason = options[:reason]
      if code
	# TODO: validate parameters.
        @context.set_reply(code, extended_code, reason)
      else
        @context.status = :temporary_failure
      end
    end

    def quarantine(reason)
      @context.qurantine(reason)
      accept
    end

    def postfix?
      @context.postfix?
    end

    def authenticated?
      @context.authenticated?
    end
  end

  class ClientSessionContext
    include MacroPredicates

    def initialize(context)
      @context = context
      @status = nil
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
      @context.macros[name]
    end

    def method_missing(*args, &block)
      @context.send(*args, &block)
    end
  end
end
