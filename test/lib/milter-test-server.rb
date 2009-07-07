#!/usr/bin/env ruby
#
# Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
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

class MilterTestServer
  def initialize
    @encoder = Milter::Encoder.new
    @decoder = Milter::ReplyDecoder.new
    @state = :start
    @socket = nil
    initialize_options
    setup_decoder
  end

  def run
    TCPSocket.open(@host, @port) do |socket|
      @socket = socket

      write(:negotiate, Milter::Option.new)
      Timeout.timeout(5) do
        while packet = @socket.readpartial(4096)
          @decoder.decode(packet)
          break if @state == :quit
        end
      end
    end
  ensure
    @socket = nil
  end

  def parse_options(argv)
    opts = OptionParser.new do |opts|
      opts.on("--host=HOST", "Connect to milter on HOST (#{@host})") do |host|
        @host = host
      end

      opts.on("--port=PORT", Integer,
              "Connect to milter on PORT (#{@port})") do |port|
        @port = port
      end

      opts.on("--mail=FILE", "Use FILE as sent mail") do |mail|
        @mail = mail
      end

      opts.on("--header=NAME,VALUE", Array, "Add header") do |name, value,|
        @headers << [name, value]
      end

      opts.on("--content=CONTENT", "Use CONTENT as mail content") do |content|
        @content = content
      end

      opts.on("--connect-host-name=NAME",
              "Use NAME as client host name connected to SMTP server") do |name|
        @connect_host_name = name
      end

      opts.on("--connect-address=ADDRESS",
              "Use ADDRESS as client address " +
              "connected to SMTP server",
              "format: inet:9999@mail.example.com, unit:/tmp/milter.sock"
              ) do |address|
        @connect_address = address
      end

      opts.on("--helo-fqdn=FQND",
              "Use FQDN as HELO SMTP command argument") do |fqdn|
        @helo_fqdn = fqdn
      end

      opts.on("--mail-from=FROM",
              "Use FROM as MAIL SMTP command argument") do |from|
        @mail_from = from
      end

      opts.on("--rcpt-to=TO",
              "Use TO as RCPT SMTP command argument") do |to|
        @rcpt_to = to
      end

      opts.on("--[no-]debug", "Output debug information") do |boolean|
        @debug = boolean
      end
    end
    opts.parse!(argv)
  end

  private
  def initialize_options
    @host = "localhost"
    @port = 9999
    @mail = nil
    @headers = []
    @content = nil
    @debug = false
    @connect_host_name = nil
    @connect_address = nil
    @helo_fqdn = nil
    @mail_from = nil
    @rcpt_to = nil
  end

  def write(encode_type, *args)
    packet, packed_size = @encoder.send("encode_#{encode_type}", *args)
    p [packet.size, packet]
    while packet
      written_size = @socket.write(packet)
      packet = packet[written_size, -1]
    end
    info("#{@state} -> #{encode_type}")
    @state = encode_type
    packed_size
  end

  def info(*args)
    p(*args) if @debug
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
      @sending_headers ||= headers
      header = @sending_headers.shift
      if header
        write(:header, *header)
      else
        write(:end_of_header)
        @sending_headers = nil
      end
    when :end_of_header, :body
      @sending_body ||= body
      if @sending_body.size > 0
        written_size = write(:body, @sending_body)
        @sending_body = @sending_body[written_size,
                                      @sending_body.size - written_size]
      else
        write(:end_of_message)
        @sending_body = nil
      end
    when :end_of_message
      write(:quit)
    else
      invalid_state(:continue)
    end
  end

  def info_add_header(name, value)
    [name, value].join(" ")
  end

  def info_insert_header(index, name, value)
    [index, name, value].join(" ")
  end

  def info_change_header(name, index, value)
    [name, index, value].join(" ")
  end

  def info_remove_header(name, index)
    [name, index].join(" ")
  end

  def info_change_from(from, parameters)
    [from, parameters].join(" ")
  end

  def info_add_recipient(recipient, parameters)
    [recipient, parameters].join(" ")
  end

  def info_delete_recipient(recipient)
    recipient
  end

  def info_replace_body(chunk)
    chunk
  end

  def info_quarantine(reason)
    reason
  end

  def sender_host_name
    @connect_host_name || Socket.gethostname
  end

  def sender_address
    if @connect_address
      return Milter::Utils.parse_connection_spec(@connect_address)[1]
    end
    type, port, host, ip_address = @socket.addr
    case type
    when "AF_INET"
      Milter::SocketAddress::IPv4.new(ip_address, port)
    when "AF_INET6"
      Milter::SocketAddress::IPv6.new(ip_address, port)
    else
      raise "unknown type: #{type}"
    end
  end

  def sender_fqdn
    @helo_fqdn || Socket.gethostname
  end

  def sender_mail_address
    return @mail_from if @mail_from
    if @mail and /^From: (.+)$/i =~ File.read(@mail)
      return $1
    end
    "kou+sender@cozmixng.org"
  end

  def receiver_mail_address
    return @rcpt_to if @rcpt_to
    if @mail and /^To: (.+)$/i =~ File.read(@mail)
      return $1
    end
    "kou+receiver@cozmixng.org"
  end

  def headers
    _headers = []
    if @mail
      header_part = File.read(@mail).split(/\r?\n\r?\n/, 2)[0]
      header_part.gsub(/\n[\t ]+/, " ").split(/\r?\n/).each do |header_line|
        _headers << header_line.split(/:\s*/, 2)
      end
    end
    _headers.concat(@headers)
    _headers
  end

  def body
    return @content if @content
    return File.read(@mail).split(/\r?\n\r?\n/, 2)[1] if @mail
    "Hi,\n\nThanks,\n"
  end

  def invalid_state(reply_state)
    write(:abort)
    write(:quit)
    raise "should not receive reply for #{reply_state} on #{@state}"
  end
end

server = MilterTestServer.new
server.parse_options(ARGV)
server.run
