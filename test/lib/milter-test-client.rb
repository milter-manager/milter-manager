#!/usr/bin/env ruby
#
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

require 'milter'
require 'socket'
require 'timeout'
require 'optparse'

class MilterTestClient
  def initialize
    @encoder = Milter::ReplyEncoder.new
    @decoder = Milter::CommandDecoder.new
    @state = :start
    @helo_fqdn = nil
    @envelope_from = nil
    @envelope_recipient = nil
    @content = ""
    @socket = nil
    @bind = true
    initialize_options
    setup_decoder
  end

  def run
    if @bind
      accept_request
    else
      print_status("ready")
      sleep(@timeout)
    end
  end

  def accept_request
    TCPServer.open(@host, @port) do |socket|
      print_status("ready")

      Timeout.timeout(@timeout) do
        @socket = socket.accept
      end

      while packet = read_packet
        @decoder.decode(packet)
        break if [:quit, :shutdown].include?(@state)
      end
    end
  ensure
    @socket = nil
  end

  def parse_options(argv)
    opts = OptionParser.new do |opts|
      opts.on("--exit", "Exit immediately") do
        exit(true)
      end

      opts.on("--host=HOST", "Listen on HOST (#{@host})") do |host|
        @host = host
      end

      opts.on("--port=PORT", Integer, "Listen on PORT (#{@port})") do |port|
        @port = port
      end

      opts.on("--[no-]print-status",
              "Print message on status change") do |boolean|
        @print_status = boolean
      end

      opts.on("--timeout=TIMEOUT", Float,
              "Use TIMEOUT as timeout value") do |timeout|
        @timeout = timeout
      end

      opts.on("--[no-]debug", "Output debug information") do |boolean|
        @debug = boolean
      end

      opts.on("--wait-second=SECOND", Integer,
              "Wait in SECOND after no_response action",
              "(#{@wait_second})") do |second|
        @wait_second = second
      end

      opts.on("--action=ACTION",
              "Do ACTION when condition is matched",
              "(#{@current_action})") do |action|
        @current_action = action # FIXME: validate
      end

      opts.on("--negotiate-version=VERSION",
              Integer,
              "Set negotiate protocol version") do |version|
        @negotiate_version = version
      end

      opts.on("--negotiate-flags=FLAG1|FLAG2|..",
              "Add step flags of negotiate option") do |flag|
        @negotiate_flags += flag.split(/\|/)
      end

      opts.on("--quarantine=REASON",
              "Send quarantine with REASON on end-of-message") do |reason|
        @end_of_message_actions << ["quarantine", reason]
      end

      opts.on("--add-header=NAME:VALUE",
              "Add a new header whose name is NAME and " +
              "value is VALUE on end-of-message") do |spec|
        @end_of_message_actions << ["add_header", *spec.split(/:/, 2)]
      end

      opts.on("--insert-header=INDEX:NAME:VALUE",
              "Insert a new header whose name is NAME and " +
              "value is VALUE at INDEX on end-of-message") do |spec|
        index, name, value = spec.split(/:/, 3)
        @end_of_message_actions << ["insert_header", Integer(index), name, value]
      end

      opts.on("--change-header=NAME:INDEX:VALUE",
              "Change a value of header whose name is NAME and " +
              "indexed by INDEX to VALUE on end-of-message") do |spec|
        name, index, value = spec.split(/:/, 3)
        @end_of_message_actions << ["change_header", name, Integer(index), value]
      end

      opts.on("--delete-header=NAME:INDEX",
              "Delete a header whose name is NAME and " +
              "indexed by INDEX on end-of-message") do |spec|
        name, index, value = spec.split(/:/, 2)
        @end_of_message_actions << ["delete_header", name, Integer(index)]
      end

      opts.on("--remove-header=NAME:INDEX",
              "Remove a header whose name is NAME and " +
              "index by INDEX on end-of-message") do |spec|
        name, index = spec.split(/:/, 2)
        @end_of_message_actions << ["remove_header", name, Integer(index)]
      end

      opts.on("--change-from=FROM",
              "Change the from adress to FROM") do |from|
        @end_of_message_actions << ["change_from", from]
      end

      opts.on("--add-recipient=RECIPIENT",
              "Send 'add-recipient' with RECIPIENT") do |recipient|
        @end_of_message_actions << ["add_recipient", recipient]
      end

      opts.on("--delete-recipient=RECIPIENT",
              "Send 'delete-recipient' with RECIPIENT") do |recipient|
        @end_of_message_actions << ["delete_recipient", recipient]
      end

      opts.on("--replace-body=CHUNK",
              "Send 'replace-body' with CHUNK") do |chunk|
        @end_of_message_actions << ["replace_body", chunk]
      end

      opts.on("--progress",
              "Send 'progress'") do
        @end_of_message_actions << ["progress"]
      end

      opts.on("--reply-code=REPLY_CODE",
              "Send 'reply-code' with REPLY_CODE") do |reply_code|
        @current_action = ["reply_code", reply_code]
      end

      opts.on("--helo=FQDN",
              "Add FQDN targets to be applied ACTION") do |fqdn|
        @helo_fqdns << [@current_action, fqdn]
      end

      opts.on("--connect-host=HOST",
              "Add HOST targets to be applied ACTION") do |host|
        @hosts << [@current_action, host]
      end

      opts.on("--envelope-from=FROM",
              "Add FROM targets to be applied ACTION") do |from|
        @senders << [@current_action, from]
      end

      opts.on("--envelope-recipient=RECIPIENT",
              "Add RECIPIENT targets to be applied ACTION") do |recipient|
        @recipients << [@current_action, recipient]
      end

      opts.on("--data", "Apply ACTION on DATA") do
        @data_actions = [@current_action]
      end

      opts.on("--header=NAME,VALUE", Array,
              "Add NAME=VALUE targets to be applied ACTION") do |name, value,|
        @headers << [@current_action, [name, value]]
      end

      opts.on("--body=CHUNK",
              "Add CHUNK targets to be applied ACTION") do |chunk|
        @body_chunks << [@current_action, chunk]
      end

      opts.on("--body-regexp=CHUNK_REGEXP",
              "Add CHUNK_REGEXP targets to be applied ACTION") do |chunk_regexp|
        @body_chunks << [@current_action, Regexp.new(chunk_regexp)]
      end

      opts.on("--end-of-message",
              "Apply ACTION on end-of-message") do
        @end_of_message_action = @current_action
      end

      opts.on("--end-of-message-chunk=CHUNK",
              "Add end-of-message CHUNK targets to be applied ACTION") do |chunk|
        @end_of_message_chunks << [@current_action, chunk]
      end

      opts.on("--end-of-message-chunk-regexp=CHUNK_REGEXP",
              "Add end-of-message CHUNK_REGEXP " \
              "targets to be applied ACTION") do |chunk_regexp|
        @end_of_message_chunks << [@current_action, Regexp.new(chunk_regexp)]
      end

      opts.on("--quit-without-reply=ACTION",
              "Quit without reply on ACTION") do |action|
        @quit_without_reply_action = action
      end

      opts.on("--no-bind",
              "Don't bind socket") do |boolean|
        @bind = boolean
      end

      opts.on("--quiet", "Be quiet") do
        ENV["MILTER_LOG_LEVEL"] = "none"
      end

      opts.on("--verbose", "Be verbose") do
        ENV["MILTER_LOG_LEVEL"] = "all"
      end
    end
    opts.parse!(argv)
  end

  private
  def initialize_options
    @host = "127.0.0.1"
    @port = 9999
    @print_status = false
    @timeout = 3
    @wait_second = 0
    @debug = false
    @current_action = "reject"
    @end_of_message_action = nil
    @end_of_message_actions = []
    @hosts = []
    @helo_fqdns = []
    @senders = []
    @recipients = []
    @data_actions = []
    @headers = []
    @body_chunks = []
    @end_of_message_chunks = []
    @negotiate_version = nil
    @negotiate_flags = ["none"]
    @quit_without_reply_action = nil
  end

  def print_status(status)
    return unless @print_status
    puts status
    $stdout.flush
  end

  def read_packet
    packet = nil
    Timeout.timeout(@timeout) do
      begin
        packet = @socket.readpartial(4096)
      rescue EOFError
      end
    end
    packet
  end

  def write(next_state, encode_type, *args)
    encode_type = encode_type.to_s.gsub(/-/, "_")
    packed_size = nil
    if encode_type == "no_response"
      next_state = :abort
      sleep(@wait_second)
    elsif need_reply(next_state)
      packet, packed_size = @encoder.send("encode_#{encode_type}", *args)
      while packet
        written_size = @socket.write(packet)
        packet = packet[written_size, -1]
      end
    end
    info("#{@state}(#{encode_type}) -> #{next_state}")
    @state = next_state
    packed_size
  end

  def need_reply(state)
    case state
    when :connected
      not @option.step.no_reply_connect?
    when :greeted
      not @option.step.no_reply_helo?
    when :envelope_from_received
      not @option.step.no_reply_envelope_from?
    when :envelope_recipient_received
      not @option.step.no_reply_envelope_recipient?
    when :data
      not @option.step.no_reply_data?
    when :header
      not @option.step.no_reply_header?
    when :end_of_header
      not @option.step.no_reply_end_of_header?
    when :body
      not @option.step.no_reply_body?
    else
      true
    end
  end

  def write_action(next_state, action)
    action, args = action
    args ||= []
    write(next_state, action, *args)
  end

  def info(*args)
    args.each {|arg| $stderr.puts(arg.inspect)} if @debug
  end

  def setup_decoder
    @decoder.class.signals.each do |signal|
      next if signal == "decode"
      @decoder.signal_connect(signal) do |_, *args|
        info(signal)
        status = "receive: #{signal}"
        normalized_signal = signal.gsub(/-/, '_')
        additional_info_callback_name = "info_#{normalized_signal}"
        if respond_to?(additional_info_callback_name, true)
          additional_info = send(additional_info_callback_name, *args)
          status << ": #{additional_info}" unless additional_info.to_s.empty?
        end
        print_status(status)
        if signal == @quit_without_reply_action
          @state = :shutdown
        else
          callback_name = "do_#{normalized_signal}"
          send(callback_name, *args) if respond_to?(callback_name, true)
        end
      end
    end
  end

  def flags_to_nicks(flags)
    flags.class.values.collect do |value|
      if (flags & value).zero?
        nil
      else
        value.nick
      end
    end.compact.join("|")
  end

  def valid_state?(current_state)
    return true if @state == :unknown

    case current_state
    when :negotiate
      @state == :start
    when :connect
      @state == :negotiate
    when :helo
      @state == :connected
    when :envelope_from
      [:greeted, :end_of_message, :abort].include?(@state)
    when :envelope_recipient
      [:envelope_from_received, :envelope_recipient_received].include?(@state)
    when :data
      @state == :envelope_recipient_received
    when :header
      [:envelope_recipient_received, :data, :header].include?(@state)
    when :end_of_header
      [:envelope_recipient_received, :data, :header].include?(@state)
    when :body
      valid_states = [:envelope_recipient_received, :data, :end_of_header, :body]
      valid_states.include?(@state)
    when :end_of_message
      valid_states = [:envelope_recipient_received, :data, :end_of_header, :body]
      valid_states.include?(@state)
    else
      info("unknown state: #{@state}")
      false
    end
  end

  def do_negotiate(option)
    invalid_state(:negotiate) unless valid_state?(:negotiate)
    @option = option
    @option.version = @negotiate_version || @option.version
    @option.step &= resolve_flags(Milter::StepFlags, @negotiate_flags)

    write(:negotiated, :negotiate, @option)
  end

  def info_negotiate(option)
    [option.version.to_s,
     flags_to_nicks(option.action),
     flags_to_nicks(option.step)].join(" ")
  end

  def do_connect(host, address)
    invalid_state(:connect) if @state != :negotiated
    @connect_host = host
    @connect_address = address

    @hosts.reverse_each do |action, _host|
      if _host == host
        write_action(:connected, action)
        return
      end
    end

    write(:connected, :continue)
  end

  def info_connect(host, address)
    [host, address.to_s].join(" ")
  end

  def info_define_macro(macro_context, macros)
    serialized_macros = macros.collect {|name, value| "#{name}: #{value}"}
    [macro_context.nick, *serialized_macros].join("\n")
  end

  def do_helo(fqdn)
    invalid_state(:helo) unless valid_state?(:helo)
    @hello_fqdn = fqdn

    @helo_fqdns.reverse_each do |action, _fqdn|
      if fqdn == _fqdn
        write_action(:greeted, action)
        return
      end
    end

    write(:greeted, :continue)
  end

  def info_helo(fqdn)
    fqdn
  end

  def do_envelope_from(from)
    invalid_state(:envelope_from) unless valid_state?(:envelope_from)
    @envelope_from = from

    @senders.reverse_each do |action, sender|
      if sender == from
        write_action(:envelope_from_received, action)
        return
      end
    end
    write(:envelope_from_received, :continue)
  end

  def info_envelope_from(from)
    from
  end

  def do_envelope_recipient(to)
    invalid_state(:envelope_recipient) unless valid_state?(:envelope_recipient)
    @envelope_recipient = to

    @recipients.reverse_each do |action, recipient|
      if recipient == to
        write_action(:envelope_recipient_received, action)
        return
      end
    end
    write(:envelope_recipient_received, :continue)
  end

  def info_envelope_recipient(to)
    to
  end

  def do_data
    invalid_state(:data) unless valid_state?(:data)

    @data_actions.reverse_each do |action|
      write_action(:data, action)
      return
    end

    write(:data, :continue)
  end

  def do_header(name, value)
    invalid_state(:header) unless valid_state?(:header)
    @headers.reverse_each do |action, (_name, _value)|
      if [name, value] == [_name, _value]
        write_action(:header, action)
        return
      end
    end

    write(:header, :continue)
  end

  def info_header(name, value)
    [name, value].join(" ")
  end

  def do_end_of_header
    invalid_state(:end_of_header) unless valid_state?(:end_of_header)

    write(:end_of_header, :continue)
  end

  def do_body(chunk)
    invalid_state(:body) unless valid_state?(:body)
    @content << chunk

    @body_chunks.each do |action, body_chunk|
      if body_chunk === chunk
        write_action(:body, action)
        return
      end
    end

    write(:body, :continue)
  end

  def info_body(chunk)
    chunk
  end

  def do_end_of_message(chunk)
    invalid_state(:end_of_message) unless valid_state?(:end_of_message)
    @end_of_message_actions.each do |action, *args|
      write(:end_of_message_actions, action, *args)
    end

    @end_of_message_chunks.each do |action, end_of_message_chunk|
      if end_of_message_chunk === chunk
        write_action(:end_of_message, action)
        return
      end
    end

    write(:end_of_message, @end_of_message_action || :continue)
  end

  def info_end_of_message(chunk)
    chunk
  end

  def do_unknown(command)
    write(:unknown, :continue)
  end

  def info_unknown(command)
    command
  end

  def do_abort
    @state = :abort
  end

  def do_quit
    @state = :quit
  end

  def invalid_state(reply_state)
    state = @state
    write(:shutdown, :shutdown)
    raise "should not receive reply for #{reply_state} on #{state}"
  end

  def resolve_flags(flags_class, flags)
    values = flags_class.values
    flags.inject(flags_class.new) do |result, flag|
      flag_value = values.find {|value| value.nick == flag}
      if flag_value.nil?
        raise "unknown flag name for #{flags_class.name}: #{flag}"
      end
      result | flag_value
    end
  end
end

ENV["MILTER_LOG_LEVEL"] = "none"
client = MilterTestClient.new
client.parse_options(ARGV)
client.run
