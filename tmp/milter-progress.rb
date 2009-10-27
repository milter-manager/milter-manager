#!/usr/bin/env ruby

base = File.dirname(__FILE__)

$LOAD_PATH.unshift("#{base}/../binding/ruby/src/toolkit/.libs")
$LOAD_PATH.unshift("#{base}/../binding/ruby/lib")

require "milter"

client = Milter::Client.new

class Session < Milter::ClientSession
  def data(context)
    p :data
    context.status = :reject
  end

  def end_of_message(context, chunk)
    p [context, chunk]
    context.add_header "hayamiZ", "X hayamiZ"
    context.add_header "hayamiZ-final", "X hayamiZ extreme"
    context.set_reply(451, "4.7.0", "DNS timeout")
  end
end

client.register(Session)

client.connection_spec = "inet:12345"

client.main

