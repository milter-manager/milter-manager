#!/usr/bin/env ruby

require 'English'

ARGF.each_line do |line|
  case line
  when /\A(\w{3} +\d+ \d+:\d+:\d+) ([\w\-]+) ([\w\d\-\/]+)(\[\d+\])?: /
    time_stamp = $1
    host = $2
    name = $3
    pid = $4
    content = $POSTMATCH

    host = "mail"
    content = content.gsub(/(?:\d{1,3}\.){3}\d{1,3}/, "192.168.1.1")
    content = content.gsub(/\[(?:[\da-f]{0,4}::?)+[\da-f]{1,4}\]/i, "[::1]")
    content = content.gsub(/(client|rhost)=\S+/, "\\1=host.example.com")
    content = content.gsub(/(orig_to)=<[\w\d\-\.=+]+>/, "\\1=<user>")
    content = content.gsub(/[\w\d\-\.=+]+@[\w\d\-\.]+/, "user@example.org")
    content = content.gsub(/[\w\d\-\.]+( ?\[[^\]]+\])/, "example.net\\1")
    content = content.gsub(/(user)=\S+/, "\\1=user")
    content = content.gsub(/(POP3)\(\S+\)/, "\\1(pop-user)")
    line = "#{time_stamp} #{host} #{name}#{pid}: #{content}"
  end
  print line
end
