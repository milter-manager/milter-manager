
#!/usr/bin/env ruby

base = File.dirname(__FILE__)

$LOAD_PATH.unshift("#{base}/../binding/ruby/src/toolkit/.libs")
$LOAD_PATH.unshift("#{base}/../binding/ruby/lib")

require "milter"

client = Milter::Client.new

client.signal_connect("connection-established") do |_client, context|
  context.signal_connect("negotiate") do |_context, option, macros_requests|
    option.step = 0
    Milter::Status::CONTINUE
  end

  require 'time'
  context.signal_connect("helo") do |*args|
    p args
    object_id = Object.new.object_id
    timeouted = false
    GLib::Timeout.add(60 * 1000) do
      p [Time.now.iso8601, :here, object_id]
      timeouted = true
      false
    end
    context = GLib::MainContext.default
    until timeouted
      p [Time.now.iso8601, :loop, object_id]
      context.iteration(true)
      p [Time.now.iso8601, timeouted, object_id]
    end
    p [Time.now, :done, object_id]
    Milter::Status::CONTINUE
  end
end

client.connection_spec = "inet:10027"

client.main

