# -*- coding: utf-8 -*-
#
# Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

require 'milter'

class Session < Milter::ClientSession
  @@sessions = []
  class << self
    def sessions
      @@sessions
    end

    def inherited(sub_class)
      super
      @@sessions << sub_class
    end
  end

  def initialize(context, options)
    super(context)
    @report_request = options.report_request
  end

  def report_request?
    @report_request
  end

  def abort(state)
    if report_request?
      puts("abort: <#{state.nick}>")
      print_macros
    end
  end

  def unknown(command)
    if report_request?
      puts("unknown: <#{command}>")
      print_macros
    end
  end

  def finished
    if report_request?
      puts("finished")
    end
  end

  private
  def inspect_flags(flags)
    flags_value = flags.to_i
    flags_klass = flags.class
    nicks = []
    flags_klass.values.each do |value|
      next if (flags_value & value).zero?
      nicks << value.nick
    end
    nicks.join("|")
  end

  def print_macros
    macros = @context.macros
    return if macros.nil?
    puts("macros:")
    macros.each do |key, value|
      puts("  #{key}=#{value}")
    end
  end
end

class NegotiateSession < Session
  def negotiate(option, macros_requests)
    if report_request?
      puts("negotiate: version=<#{option.version}>, " +
           "action=<#{inspect_flags(option.action)}>, " +
           "step=<#{inspect_flags(option.step)}>")
      print_macros
    end
    super
    option.remove_step(Milter::StepFlags::ENVELOPE_RECIPIENT_REJECTED)
  end
end

class ConnectSession < Session
  def connect(host, address)
    if report_request?
      puts("connect: host=<#{host}>, address=<#{address}>")
      print_macros
    end
  end
end

class HeloSession < Session
  def helo(fqdn)
    if report_request?
      puts("helo: <#{fqdn}>")
      print_macros
    end
  end
end

class EnvelopeFromSession < Session
  def envelope_from(from)
    if report_request?
      puts("envelope-from: <#{from}>")
      print_macros
    end
  end
end

class EnvelopeRecipientSession < Session
  def envelope_recipient(to)
    if report_request?
      puts("envelope-recipient: <#{to}>")
      print_macros
    end
  end
end

class DataSession < Session
  def data
    if report_request?
      puts("data")
      print_macros
    end
  end
end

class HeaderSession < Session
  def header(name, value)
    if report_request?
      puts("header: <#{name}: #{value}>")
      print_macros
    end
  end
end

class EndOfHeaderSession < Session
  def end_of_header
    if report_request?
      puts("end-of-header")
      print_macros
    end
  end
end

class BodySession < Session
  def body(chunk)
    if report_request?
      puts("body: <#{chunk}>")
      print_macros
    end
  end
end

class EndOfMessageSession < Session
  def end_of_message
    if report_request?
      puts("end-of-message")
      print_macros
    end
  end
end

command_line = Milter::Client::CommandLine.new

options = command_line.options
options.report_request = true

option_parser = command_line.option_parser
option_parser.separator ""
option_parser.separator "milter-test-client-composite specific options"
option_parser.on("--no-report-request",
                 "Don't report request values") do |bool|
  options.report_request = bool
end

command_line.run do |client, _options|
  client.register(Milter::Client::CompositeSession,
                  Session.sessions,
                  _options)
  if _options.report_request
    client.on_maintain do
      puts("maintain")
    end
  end
end
