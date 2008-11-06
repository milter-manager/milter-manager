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

      opts.on("--action=ACTION",
              "Do ACTION when condition is matched",
              "(#{@current_action})") do |action|
        @current_action = action # FIXME: validate
      end

      opts.on("--quarantine=REASON",
              "Send quarantine with REASON on end-of-message") do |reason|
        @end_of_message_actions << [:quarantine, reason]
      end

      opts.on("--add-header=NAME:VALUE",
              "Add a new header whose name is NAME and " +
              "value is VALUE on end-of-message") do |name_and_value|
        @end_of_message_actions << [:add_header, *name_and_value.split(/:/, 2)]
      end

      opts.on("--change-from=FROM",
              "Change the from adress to FROM") do |from|
        @end_of_message_actions << [:change_from, from]
      end

      opts.on("--add-recipient=RECIPIENT",
              "Send 'add-recipient' with RECIPIENT") do |recipient|
        @end_of_message_actions << [:add_recipient, recipient]
      end

      opts.on("--delete-recipient=RECIPIENT",
              "Send 'delete-recipient' with RECIPIENT") do |recipient|
        @end_of_message_actions << [:delete_recipient, recipient]
      end

      opts.on("--replace-body=CHUNK",
              "Send 'replace-body' with CHUNK") do |chunk|
        @end_of_message_actions << [:replace_body, chunk]
      end

      opts.on("--reply-code=REPLY_CODE",
              "Send 'reply-code' on end-of-message") do |reply_code|
        @reply_code = reply_code
        @current_action = "reject"
      end

      opts.on("--progress",
              "Send 'progress'") do
        @end_of_message_actions << [:progress]
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
              "Add CHUNK targets to be applied ACTION") do |body_chunk|
        @body_chunks << [@current_action, body_chunk]
      end

      opts.on("--end-of-message=CHUNK",
              "Add CHUNK targets to be applied ACTION") do |chunk|
        @end_of_message_chunks << [@current_action, chunk]
      end
    end
    opts.parse!(argv)
  end

  private
  def initialize_options
    @port = 9999
    @print_ready = false
    @timeout = 3
    @debug = false
    @current_action = "reject"
    @end_of_message_actions = []
    @recipients = []
    @senders = []
    @body_chunks = []
    @reply_code = nil
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
      packet = @socket.readpartial(4096)
    end
    packet
  end

  def write(next_state, encode_type, *args)
    packet, packed_size = @encoder.send("encode_reply_#{encode_type}", *args)
    while packet
      written_size = @socket.write(packet)
      packet = packet[written_size, -1]
    end
    info("#{@state}(#{encode_type}) -> #{next_state}")
    @state = next_state
    packed_size
  end

  def write_action(next_state, action)
    args = []
    case action.to_s
    when "quarantine" # NO NEED
      args << @quarantine_reason
    when "reject"
      if @reply_code
        args << @reply_code
        action = "reply_code"
      end
    end
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

  def do_negotiate(option)
    invalid_state(:negotiate) if @state != :start
    @option = option

    write(:negotiated, :negotiate, option)
  end

  def do_connect(host, address)
    invalid_state(:connect) if @state != :negotiated
    @connect_host = host
    @connect_address = address

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
      if body_chunk == chunk
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
      if end_of_message_chunk == chunk
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
