#!/usr/bin/env ruby

unless ARGV.empty?
  puts <<HELP
How to run

Edit /etc/hosts as following:

/etc/hosts:
   ...
   # add an entry to specify emulation server
   192.168.1.5 sourceforge.net
   ...

Run server:
   $ sudo ./emurate-repository.rb
HELP
  exit
end

require "webrick"

server = WEBrick::HTTPServer.new(:BindAddress => "0.0.0.0",
                                 :Port => 80)

server.mount("/projects/milter-manager/files/",
             WEBrick::HTTPServlet::FileHandler,
             "./")
trap("INT") do
  server.shutdown
end
server.start
