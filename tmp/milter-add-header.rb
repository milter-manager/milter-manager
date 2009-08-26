
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
  
  context.signal_connect("end-of-message") do |*args|
    p args
    context.add_header "hayamiZ", "X hayamiZ"
    p context.progress
    context.add_header "hayamiZ-final", "X hayamiZ extreme"
    Milter::Status::CONTINUE
  end
end

client.connection_spec = "inet:10027"

client.main

