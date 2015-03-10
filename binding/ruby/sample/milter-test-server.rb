#!/usr/bin/env ruby
#
#  Copyright (C) 2015  Kenji Okimoto <okimoto@clear-code.com>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

require "optparse"
require "pathname"
require "milter/server"

class MilterTestServer
  Message = Struct.new("Message",
                       :envelope_from,
                       :original_envelope_from,
                       :recipients,
                       :original_recipients,
                       :heaers,
                       :original_headers,
                       :body,
                       :replaced_body)

  ProcessData = Struct.new("ProcessData",
                           :loop,
                           :timer,
                           :succeeded,
                           :reply_code,
                           :reply_extended_code,
                           :reply_message,
                           :quarantine_reason,
                           :option,
                           :option_headers,
                           :current_body_chunk,
                           :current_recipient,
                           :body_chunks,
                           :message,
                           :error)

  class Error < StandardError
  end

  def initialize
    @negotiate_version = 6
    @connect_macros = {}
    @helo_fqdn = ""
    @helo_macros = {}
    @envelope_from = ""
    @envelope_from_macros = {}
    @envelope_recipients = []
    @envelope_recipient_macros = {}
    @data_macros = {}
    @headers = {}
    @end_of_header_macros = {}
    @chunks = []
    @end_of_message_macros = {}
    prepare_parser
  end

  def run(argv)
    ENV["MILTER_LOG_LEVEL"] = "all" if @verbose
    parse_options(argv)
    apply_options_to_macros
    data = init_process_data
    test_server_thread(data)
  end

  private

  def normalize_envelope_address(address)
    address.gsub(/<?(.+)>?/){  "<#{$1}>" }
  end

  def apply_options_to_macros
    @connect_macros["daemon_name"] = $0
    @connect_macros["if_name"] = "localhost"
    @connect_macros["if_addr"] = "127.0.0.1"
    @connect_macros["j"] = "mail.example.com"

    @helo_macros["tls_version"] = "0"
    @helo_macros["cipher"] = "0"
    @helo_macros["cipher_bits"] = "0"
    @helo_macros["cert_subject"] = "cert_subject"
    @helo_macros["cert_issue"] = "cert_issue"

    @envelope_from_macros["i"] = "i"
    @envelope_from_macros["mail_mailer"] = "mail_mailer"
    @envelope_from_macros["mail_host"] = "mail_host"
    @envelope_from_macros["mail_addr"] = "mail_addr"
    @envelope_from_macros["auth_authen"] = @authenticated_name if @authenticated_name
    @envelope_from_macros["auth_type"] = @authenticated_type if @authenticated_type
    @envelope_from_macros["auth_author"] = @authenticated_author if @authenticated_author

    @envelope_recipient_macros["rcpt_mailer"] = "rcpt_mailer"
    @envelope_recipient_macros["rcpt_host"] = "rcpt_host"

    @end_of_message_macros["i"] = "message-id"
  end

  def init_process_data
    data = ProcessData.new
    data.loop = Milter::GLibEventLoop.new
    data.timer = GLib::Timer.new
    data.succeeded = true
    data.quarantine_reason = nil
    data.option = nil
    data.option_headers = @headers.each
    data.current_body_chunk = @chunks.each
    data.current_recipient = @envelope_recipients.each
    data.reply_code = 0
    data.reply_extended_code = nil
    data.reply_message = nil
    data.message = Message.new
    data.error = nil
    data
  end

  def test_server_thread(data)
    context = Milter::ServerContext.new
    setup_context(context, data)
    start_process(context, data)
  end

  def setup_context(context, data)
    context.event_loop = data.loop
    context.name = $0
    context.connection_timeout = @connection_timeout if @connection_timeout
    context.reading_timeout = @reading_timeout if @reading_timeout
    context.writing_timeout = @writing_timeout if @writing_timeout
    context.end_of_message_timeout = @end_of_message_timeout if @end_of_message_timeout

    context.signal_connect("ready", data) do |_context, _data|
      setup(_context, _data)
      data.timer.start
      negotiate(_context)
    end
    context.signal_connect("error", data) do |_context, error, _data|
      _data.succeeded = false
      _data.error = error
      data.loop.quit
    end
  end

  def setup(context, data)
    context.signal_connect("negotiate_reply", data) do |_context, _option, _macros_request, _data|
      if _data.option
        send_abort(_context, _data)
      else
        _data.option = _option
        context.signal_emit("continue")
      end
    end
    context.signal_connect("continue", data) do |_context, _data|
      case _context.state.nick
      when "negotiate"
        send_connect(_context, _data)
      when "connect"
        send_helo(_context, _data)
      when "helo"
        send_envelope_from(_context, _data)
      when "envelope-from"
        send_envelope_recipient(_context, _data)
      when "envelope-recipient"
        next if send_envelope_recipient(_context, _data)
        next if send_unknown(_context, _data)
        send_data(_context, _data)
      when "unknown"
        send_data(_context, _data)
      when "data"
        send_header(_context, _data)
      when "header"
        next if send_header(_context, _data)
        send_end_of_header(_context, _data)
      when "end-of-header"
        send_body(_context, _data)
      when "body"
        next if send_body(_context, _data)
        send_end_of_message(_context, _data)
      when "end-of-message"
        send_quit(_context, _data)
      else
        # milter_error
        send_abort(_context, _data)
      end
    end
    context.signal_connect("reply_code", data) do |_context, code, extended_code, message, _data|
      _data.reply_code = code
      _data.reply_extended_code = extended_code
      _data.reply_message = message

      if (400..500).include?(code)
        context.signal_emit("temporary_failure", _data)
      else
        context.signal_emit("reject", _data)
      end
    end
    context.signal_connect("temporary_failure", data) do |_context, _data|
      case _context.state.nick
      when "envelope_recipient"
        unless data.message.recipients.empty?
          context.signal_emit("continue", _data)
        end
      else
        send_abort(_context, _data)
        data.succeeded = false
      end
    end
    context.signal_connect("reject", data) do |_context, _data|
      case _context.state.nick
      when "envelope_recipient"
        unless data.message.recipients.empty?
          context.signal_emit("continue", _data)
        end
      else
        send_abort(_context, _data)
        data.succeeded = false
      end
    end
    context.signal_connect("accept", data) do |_context, _data|
      send_abort(_contect, _data)
      _data.succeeded = true
    end
    context.signal_connect("discard", data) do |_context, _data|
      send_abort(_contect, _data)
      _data.succeeded = false
    end
    context.signal_connect("add_header", data) do |_context, _data|
      # TODO
    end
    context.signal_connect("insert_header", data) do |_context, _data|
      # TODO
    end
    context.signal_connect("change_header", data) do |_context, _data|
      # TODO
    end
    context.signal_connect("delete_header", data) do |_context, _data|
      # TODO
    end
    context.signal_connect("change_from", data) do |_context, from, parameters, _data|
      _data.message.envelope_from = from
    end
    context.signal_connect("add_recipient", data) do |_context, recipient, parameters, _data|
      _dtat.message.recipients << recipient
    end
    context.signal_connect("delete_recipient", data) do |_context, recipient, _data|
      _data.message.recipients.delete(recipient)
    end
    context.signal_connect("replace_body", data) do |_context, body, body_size, _data|
      _data.message.replaced_body = body
    end
    context.signal_connect("progress", data) do |_context, _data|
      # do nothing
    end
    context.signal_connect("quarantine", data) do |_context, _data|
      send_abort(_context, _data)
    end
    context.signal_connect("connection_failure", data) do |_context, _data|
      send_abort(_context, _data)
    end
    context.signal_connect("shutdown", data) do |_context, _data|
      send_abort(_context, _data)
    end
    context.signal_connect("skip", data) do |_context, _data|
      begin
        loop do
          _data.current_body_chunk.next
        end
      rescue StopIteration
        # Do nothing
      ensure
        _context.signal_emit("continue", _data)
      end
    end
    context.signal_connect("error", data) do |_context, error, _data|
      _data.error = error
      _context.shutdown
    end
    context.signal_connect("finished", data) do |_context, _data|
      _data.timer.stop
      _data.loop.quit
    end

    context.signal_connect("connection_timeout", data) do |_context, _data|
      _data.succeeded = false
      _data.error = Error.new("connection timeout")
    end
    context.signal_connect("writing_timeout", data) do |_context, _data|
      _data.succeeded = false
      _data.error = Error.new("writing timeout")
    end
    context.signal_connect("reading_timeout", data) do |_context, _data|
      _data.succeeded = false
      _data.error = Error.new("reading timeout")
    end
    context.signal_connect("end_of_message_timeout", data) do |_context, _data|
      _data.succeeded = false
      _data.error = Error.new("end_of_message timeout")
    end

    context.signal_connect("state_transited", data) do |_context, state, _data|
      step = Milter::StepFlags::NONE
      step = data.option.step if data.option

      case state.nick
      when "connect"
        next unless step == Milter::StepFlags::NO_REPLY_CONNECT
        next if send_helo(_context, _data)
      when "helo"
        next unless step == Milter::StepFlags::NO_REPLY_HELO
        next if send_envelope_from(_context, _data)
      when "envelope-from"
        next unless step == Milter::StepFlags::NO_REPLY_ENVELOPE_FROM
        next if send_envelope_recipient(_context, _data)
      when "envelope-recipient"
        next unless step == Milter::StepFlags::NO_REPLY_ENVELOPE_RECIPIENT
        next if send_envelope_recipient(_context, _data)
        next if send_unknown(_context, _data)
        next if send_data(_context, _data)
      when "unknown"
        next unless step == Milter::StepFlags::NO_REPLY_UNKNOWN
        next if send_data(_context, _data)
      when "data"
        next unless step == Milter::StepFlags::NO_REPLY_DATA
        next if send_header(_context, _data)
      when "header"
        next unless step == Milter::StepFlags::NO_REPLY_HEADER
        next if send_header(_context, _data)
        next if send_end_of_header(_context, _data)
      when "end-of-header"
        next unless step == Milter::StepFlags::NO_REPLY_END_OF_HEADER
        next if send_body(_context, _data)
      when "body"
        next unless step == Milter::StepFlags::NO_REPLY_BODY
        next if send_end_of_message(_context, _data)
      else
        next
      end
    end
  end

  def start_process(context, data)
    context.connection_spec = @connection_spec || "inet:10025"
    context.establish_connection
    context.event_loop.run
    print_result(context, data)
  end

  def print_result(context, data)
    puts "done"
  end

  def negotiate(context)
    option = Milter::Option.new(@negotiate_version,
                                Milter::ActionFlags::ADD_HEADERS |
                                Milter::ActionFlags::CHANGE_BODY |
                                Milter::ActionFlags::ADD_ENVELOPE_RECIPIENT |
                                Milter::ActionFlags::DELETE_ENVELOPE_RECIPIENT |
                                Milter::ActionFlags::CHANGE_HEADERS |
                                Milter::ActionFlags::QUARANTINE |
                                Milter::ActionFlags::CHANGE_ENVELOPE_FROM |
                                Milter::ActionFlags::ADD_ENVELOPE_RECIPIENT_WITH_PARAMETERS |
                                Milter::ActionFlags::SET_SYMBOL_LIST,
                                Milter::StepFlags::NO_CONNECT |
                                Milter::StepFlags::NO_HELO |
                                Milter::StepFlags::NO_ENVELOPE_FROM |
                                Milter::StepFlags::NO_ENVELOPE_RECIPIENT |
                                Milter::StepFlags::NO_BODY |
                                Milter::StepFlags::NO_HEADERS |
                                Milter::StepFlags::NO_END_OF_HEADER |
                                Milter::StepFlags::NO_REPLY_HEADER |
                                Milter::StepFlags::NO_UNKNOWN |
                                Milter::StepFlags::NO_DATA |
                                Milter::StepFlags::SKIP |
                                Milter::StepFlags::ENVELOPE_RECIPIENT_REJECTED |
                                Milter::StepFlags::NO_REPLY_CONNECT |
                                Milter::StepFlags::NO_REPLY_HELO |
                                Milter::StepFlags::NO_REPLY_ENVELOPE_FROM |
                                Milter::StepFlags::NO_REPLY_ENVELOPE_RECIPIENT |
                                Milter::StepFlags::NO_REPLY_DATA |
                                Milter::StepFlags::NO_REPLY_UNKNOWN |
                                Milter::StepFlags::NO_REPLY_END_OF_HEADER |
                                Milter::StepFlags::NO_REPLY_BODY |
                                Milter::StepFlags::HEADER_VALUE_WITH_LEADING_SPACE)
    context.negotiate(option)
  end

  def send_connect(context, data)
    context.set_macros(Milter::Command::CONNECT, @connect_macros)
    return false if data.option.step == Milter::StepFlags::NO_CONNECT

    host_name = @connect_host || "mx.example.net"
    address_spec = @connect_address || "inet:50443@192.168.123.123"
    context.connect(host_name, address_spec)
  end

  def send_helo(context, data)
    context.set_macros(Milter::Command::HELO, @helo_macros)
    return false if data.option.step == Milter::StepFlags::NO_HELO

    context.helo(@helo_fqdn)
  end

  def send_envelope_from(context, data)
    context.reset_message_related_data()
    context.set_macros(Milter::Command::ENVELOPE_FROM, @envelope_from_macros)
    return false if data.option.step == Milter::StepFlags::NO_ENVELOPE_FROM

    context.envelope_from(@envelope_from)
  end

  def send_envelope_recipient(context, data)
    recipient = data.current_recipient.next
    context.set_macros(Milter::Command::ENVELOPE_RECIPIENT, @envelope_recipient_macros)
    return false if data.option.step == Milter::StepFlags::NO_ENVELOPE_RECIPIENT

    context.envelope_recipient(recipient)
  rescue StopIteration
    false
  end

  def send_unknown(context, data)
    return false if data.option.step == Milter::StepFlags::NO_UNKNOWN
    return false unless @unknown_command
    return false if data.option.version < 0

    context.unknown(@unknown_command)
  end

  def send_data(context, data)
    context.set_macros(Milter::Command::DATA, @data_macros)
    return false if data.option.step == Milter::StepFlags::NO_DATA
    return false if data.option.version < 4

    context.data
  end

  def send_header(context, data)
    return false if data.option.step == Milter::StepFlags::NO_HEADERS
    name, value = data.option_headers.next
    context.header(name, value)
  rescue StopIteration
    false
  end

  def send_end_of_header(context, data)
    context.set_macros(Milter::Command::END_OF_HEADER, @end_of_header_macros)
    return false if data.option.step == Milter::StepFlags::NO_END_OF_HEADER

    context.end_of_header
  end

  def send_body(context, data)
    return false if data.option.step == Milter::StepFlags::NO_BODY
    body_chunk = data.current_body_chunk.next
    context.body(body_chunk)
  rescue StopIteration
    false
  end

  def send_end_of_message(context, data)
    context.set_macros(Milter::Command::END_OF_MESSAGE, @end_of_message_macros)
    context.end_of_message
  end

  def send_quit(context, data)
    context.quit
    context.shutdown
  end

  def send_abort(context, data)
    context.abort
    send_quit(context, data)
  end

  def prepare_parser
    @parser = OptionParser.new
    @parser.banner = "#{$0} [options...]"
    @parser.on("--name=NAME",
               "Use NAME as the milter server name") do |name|
      @name = name
    end
    @parser.on("-s", "--connection-spec=SPEC",
               "The spec of client socket. (unix:PATH|inet:PORT[@HOST]|inet6:PORT[@HOST])") do |spec|
      @connection_spec = spec
    end
    @parser.on("--negotiate-version=VERSION", Integer,
               "Use VERSION as milter protocol version on negotiate. (6)") do |version|
      @negotiate_version = version
    end
    @parser.on("--connect-host=HOST",
               "Use HOST as host name on connect") do |host|
      @connect_host = host
    end
    @parser.on("--connect-address=SPEC",
               "Use SPEC for address on connect. (unix:PATH|inet:PORT[@HOST]|inet6:PORT[@HOST])") do |spec|
      @connect_address = spec
    end
    @parser.on("--connect-macro=NAME:VALUE",
               "Add a macro that has NAME name and VALUE value on xxfi_connect()." +
               " To add N macros, use --connect-macro option N times.") do |macro|
      key, value = macro.split(":")
      @connect_macros[key] = value
    end
    @parser.on("--helo-fqdn=FQDN", "Use FQDN for HELO/EHLO command") do |fqdn|
      @helo_fqdn = fqdn
    end
    @parser.on("--helo-macro=NAME:VALUE",
               "Add a macro that has NAME name and VALUE value on xxfi_helo()." +
               " To add N macros, use --helo-macro option N times.") do |macro|
      key, value = macro.split(":")
      @helo_macros[key] = value
    end
    @parser.on("-f", "--envelope-from=FROM ", "Use FROM as a sender address") do |from|
      @envelope_from = normalize_envelope_address(from)
    end
    @parser.on("--envelope-from-macro=NAME:VALUE",
               "Add a macro that has NAME name and VALUE value on xxfi_envfrom()." +
               " To add N macros, use --envelope-from-macro option N times.") do |macro|
      key, value = macro.split(":")
      @envelope_from_macros[key] = value
    end
    @parser.on("-r", "--envelope-recipient=RECIPIENT",
               "Add a recipient RECIPIENT. To add N recipients," +
               " use --envelope-recipient option N times.") do |recipient|
      @envelope_recipients << recipient
    end
    @parser.on("--envelope-recipient-macro=NAME:VALUE",
               "Add a macro that has NAME name and VALUE value on xxfi_envrcpt()." +
               " To add N macros, use --envelope-recipient-macro option N times.") do |macro|
      key, value = macro.split(":")
      @envelope_recipient_macros[key] = value
    end
    @parser.on("--data-macro=NAME:VALUE",
               "Add a macro that has NAME name and VALUE value on xxfi_data()." +
               " To add N macros, use --data-macro option N times.") do |macro|
      key, value = macro.split(":")
      @data_macros[key] = value
    end
    @parser.on("-h", "--header=NAME:VALUE",
               "Add a header. To add n headers, use --header option n times.") do |header|
      name, value = header.split(":")
      @headers[name] = value
    end
    @parser.on("--end-of-header-macro=NAME:VALUE",
               "Add a macro that has NAME name and VALUE value on xxfi_eoh()." +
               " To add N macros, use --end-of-header-macro option N times.") do |macro|
      key, value = macro.split(":")
      @end_of_header_macros[key] = value
    end
    @parser.on("-b", "--body=CHUNK",
               "Add a body chunk. To add n body chunks, use --body option n times.") do |chunk|
      @chunks << chunk
    end
    @parser.on("--end-of-message-macro=NAME:VALUE",
               "Add a macro that has NAME name and VALUE value on xxfi_eom()." +
               " To add N macros, use --end-of-message-macro option N times.") do |macro|
      key, value = macro.split(":")
      @end_of_message_macros[key] = value
    end
    @parser.on("--unknown=COMMAND", "Use COMMAND for unknown SMTP command.") do |command|
      @unknown_command = command
    end
    @parser.on("--authenticated-name=NAME",
               "Use NAME for authenticated SASL login name." +
               " (This is used for {auth_authen} macro value.)") do |name|
      @authenticated_name = name
    end
    @parser.on("--authenticated-type=TYPE",
               "Use TYPE for authenticated SASL login method." +
               " (This is used for {auth_type} macro value.)") do |type|
      @authenticated_type = type
    end
    @parser.on("--authenticated-author=AUTHOR",
               "Use AUTHOR for authenticated SASL sender." +
               " (This is used for {auth_author} macro value.)") do |author|
      @authenticated_author = author
    end
    @parser.on("-m, --mail-file=PATH", "Use mail placed at PATH as mail content.") do |path|
      @mail_file = Pathname(path).realpath
      parse_mail_contents(@mail_file.read)
    end
    @parser.on("--output-message", "Output modified message") do
      @output_message = true
    end
    # @parser.on("--color=[yes|true|no|false|auto]", "Output modified message with colors")
    @parser.on("--connection-timeout=SECONDS",
               "Timeout after SECONDS seconds on connecting to a milter.") do |timeout|
      @connection_timeout = timeout
    end
    @parser.on("--reading-timeout=SECONDS",
               "Timeout after SECONDS seconds on reading a command.") do |timeout|
      @reading_timeout = timeout
    end
    @parser.on("--writing-timeout=SECONDS",
               "Timeout after SECONDS seconds on writing a command.") do |timeout|
      @writing_timeout = timeout
    end
    @parser.on("--end-of-message-timeout=SECONDS",
               "Timeout after SECONDS seconds on end-of-message command.") do |timeout|
      @end_of_message_timeout = timeout
    end
    # @parser.on("-t", "--threads=N", "Create N threads.")
    @parser.on("-v", "--verbose", "Be verbose") do
      @verbose = true
    end
  end

  def parse_options(argv)
    prepare_parser
    begin
      @parser.parse!(argv)
    rescue OptionParser::ParseError => ex
      $stderr.puts ex.class
      $stderr.puts ex.message
      exit false
    end

    if @helo_fqdn.empty?
      @helo_fqdn = "delian"
    end
    if @envelope_from.empty?
      @envelope_from = normalize_envelope_address("sender@example.com")
    end
    if @envelope_recipients.empty?
      @envelope_recipients << "receiver@example.org"
    end
    @envelope_recipients = @envelope_recipients.map do |recipient|
      normalize_envelope_address(recipient)
    end
    unless @headers.has_key?("From")
      @headers["From"] = @envelope_from
    end
    unless @headers.has_key?("To")
      @headers["To"] = @envelope_recipients.first
    end
    if @chunks.empty?
      @chunks << "La de da de da 1.\n" +
        "La de da de da 2.\n" +
        "La de da de da 3.\n" +
        "La de da de da 4."
    end
  end

  def parse_mail_contents(contents)
  end
end

MilterTestServer.new.run(ARGV)
