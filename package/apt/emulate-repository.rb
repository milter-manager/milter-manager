#!/usr/bin/env ruby
#
# How to run
#
# Edit /etc/hosts as following:
#
# /etc/hosts:
#   ...
#   # add an entry to specify emulation server
#   192.168.1.5 downloads.sourceforge.net
#   ...
#
# Run server:
#   $ sudo ./emurate-repository.rb
#

require "webrick"

server = WEBrick::HTTPServer.new(:BindAddress => "0.0.0.0",
                                 :Port => 80)

server.mount("/project/milter-manager/", WEBrick::HTTPServlet::FileHandler, "./")
trap("INT") do
  server.shutdown
end
server.start
