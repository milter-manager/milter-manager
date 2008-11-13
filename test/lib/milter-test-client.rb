#!/usr/bin/env ruby

require 'milter'
require 'socket'
require 'timeout'
require 'optparse'

class MilterTestClient
  def initialize
    @encoder = Milter::Encoder.new
    @decoder = Milter::CommandDecoder.new
    @state = :start
    @helo_fqdn = nil
    @mail_from = nil
    @rcpt_to = nil
    @headers = []
    @content = ""
    @socket = nil
    initialize_options
    setup_decoder
  end

  def run
    TCPServer.open(@port) do |socket|
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

      opts.on("--connect-host=HOST",
              "Add HOST targets to be applied ACTION") do |host|
        @hosts << [@current_action, host]
      end

      opts.on("--envelope-recipient=RECIPIENT",
              "Add RECIPIENT targets to be applied ACTION") do |recipient|
        @recipients << [@current_action, recipient]
      end

      opts.on("--envelope-from=FROM",
              "Add FROM targets to be applied ACTION") do |from|
        @senders << [@current_action, from]
      end

      opts.on("--body=CHUNK",
              "Add CHUNK targets to be applied ACTION") do |chunk|
        @body_chunks << [@current_action, chunk]
      end

      opts.on("--body-regexp=CHUNK_REGEXP",
              "Add CHUNK_REGEXP targets to be applied ACTION") do |chunk_regexp|
        @body_chunks << [@current_action, Regexp.new(chunk_regexp)]
      end

      opts.on("--end-of-message=CHUNK",
              "Add CHUNK targets to be applied ACTION") do |chunk|
        @end_of_message_chunks << [@current_action, chunk]
      end

      opts.on("--end-of-message-regexp=CHUNK_REGEXP",
              "Add CHUNK_REGEXP targets to be applied ACTION") do |chunk_regexp|
        @end_of_message_chunks << [@current_action, Regexp.new(chunk_regexp)]
      end
    end
    opts.parse!(argv)
  end

  private
  def initialize_options
    @port = 9999
    @print_ready = false
    @timeout = 3
    @wait_second = 0
    @debug = false
    @current_action = "reject"
    @end_of_message_actions = []
    @hosts = []
    @recipients = []
    @senders = []
    @body_chunks = []
    @end_of_message_chunks = []
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
    else
      packet, packed_size = @encoder.send("encode_reply_#{encode_type}", *args)
      while packet
        written_size = @socket.write(packet)
        packet = packet[written_size, -1]
      end
    end
    info("#{@state}(#{encode_type}) -> #{next_state}")
    @state = next_state
    packed_size
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
      next if signal == "decode-command"
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
        callback_name = "do_#{normalized_signal}"
        send(callback_name, *args) if respond_to?(callback_name, true)
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

  def do_negotiate(option)
    invalid_state(:negotiate) if @state != :start
    @option = option

    write(:negotiated, :negotiate, option)
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

  def do_helo(fqdn)
    invalid_state(:helo) if @state != :connected
    @hello_fqdn = fqdn

    write(:greeted, :continue)
  end

  def info_helo(fqdn)
    fqdn
  end

  def do_mail(from)
    invalid_state(:mail) if @state != :greeted
    @mail_from = from

    @senders.reverse_each do |action, sender|
      if sender == from
        write_action(:mailed, action)
        return
      end
    end
    write(:mailed, :continue)
  end

  def info_mail(from)
    from
  end

  def do_rcpt(to)
    invalid_state(:rcpt) unless [:mailed, :recipiented].include?(@state)
    @rcpt_to = to

    @recipients.reverse_each do |action, recipient|
      if recipient == to
        write_action(:recipiented, action)
        return
      end
    end
    write(:recipiented, :continue)
  end

  def info_rcpt(to)
    to
  end

  def do_data
    invalid_state(:data) if @state != :recipiented

    write(:data, :continue)
  end

  def do_header(name, value)
    invalid_state(:header) unless [:data, :header].include?(@state)
    @headers << [name, value]

    write(:header, :continue)
  end

  def info_header(name, value)
    [name, value].join(" ")
  end

  def do_end_of_header
    invalid_state(:end_of_header) unless [:data, :header].include?(@state)

    write(:end_of_header, :continue)
  end

  def do_body(chunk)
    invalid_state(:body) unless [:end_of_header, :body].include?(@state)
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
    unless [:end_of_header, :body].include?(@state)
      invalid_state(:end_of_message)
    end

    @end_of_message_actions.each do |action, *args|
      write(:end_of_message_actions, action, *args)
    end

    @end_of_message_chunks.each do |action, end_of_message_chunk|
      if end_of_message_chunk === chunk
        write_action(:end_of_message, action)
        return
      end
    end

    write(:end_of_message, :continue)
  end

  def info_end_of_message(chunk)
    chunk
  end

  def do_unknown(command)
    write(:unknown, :reject)
  end

  def info_unknown(command)
    command
  end

  def do_abort
    @state = :abort
  end

  def info_abort

  end

  def do_quit
    @state = :quit
  end

  def invalid_state(reply_state)
    state = @state
    write(:shutdown, :shutdown)
    raise "should not receive reply for #{reply_state} on #{state}"
  end
end

client = MilterTestClient.new
client.parse_options(ARGV)
client.run
