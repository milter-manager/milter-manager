#!/usr/bin/env ruby

require 'milter'
require 'socket'

class MilterTestServer
  def initialize
    @encoder = Milter::Encoder.new
    @decoder = Milter::ReplyDecoder.new
    @state = :start
    @headers = [["From", sender_mail_address],
                ["To", receiver_mail_address]]
    @socket = nil
    setup_decoder
  end

  def run
    TCPSocket.open("localhost", 9999) do |socket|
      @socket = socket

      write(:negotiate, Milter::Option.new)
      while packet = @socket.readpartial(4096)
        @decoder.decode(packet)
        break if @state == :quit
      end
    end
  ensure
    @socket = nil
  end

  private
  def write(encode_type, *args)
    packet, packed_size = @encoder.send("encode_#{encode_type}", *args)
    size = packet.size
    while size > 0
      written_size = @socket.write(packet)
      size -= written_size
      packet = packet[written_size, -1]
    end
    p "#{@state} -> #{encode_type}"
    @state = encode_type
  end

  def setup_decoder
    @decoder.class.signals.each do |signal|
      @decoder.signal_connect(signal) do |_, *args|
        p signal
        callback_name = "do_#{signal.gsub(/-/, '_')}"
        send(callback_name, *args) if respond_to?(callback_name, true)
      end
    end
  end

  def do_negotiate_reply(option, macros_requests)
    invalid_state(:negotiate_reply) if @state != :negotiate
    @option = option
    @macro_requests = macros_requests

    write(:connect, sender_host_name, sender_address)
  end

  def do_continue
    case @state
    when :connect
      write(:helo, sender_fqdn)
    when :helo
      write(:mail, sender_mail_address)
    when :mail
      write(:rcpt, receiver_mail_address)
    when :rcpt
      write(:data)
    when :data, :header
      header = @headers.pop
      if header
        write(:header, *header)
      else
        write(:end_of_header)
      end
    when :end_of_header
      write(:body, "Hi,\n\nThanks,\n")
    when :body
      write(:end_of_message)
    when :end_of_message
      write(:quit)
    else
      invalid_state(:continue)
    end
  end

  def sender_host_name
    Socket.gethostname
  end

  def sender_address
    Socket.sockaddr_in(*@socket.addr.values_at(1, 3))
  end

  def sender_fqdn
    Socket.gethostname
  end

  def sender_mail_address
    "kou+sender@cozmixng.org"
  end

  def receiver_mail_address
    "kou+receiver@cozmixng.org"
  end

  def invalid_state(reply_state)
    write(:abort)
    write(:quit)
    raise "should not receive reply for #{reply_state} on #{@state}"
  end
end

server = MilterTestServer.new
server.run
