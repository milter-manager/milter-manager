#!/usr/bin/env ruby

if ARGV.size < 4
  puts "Usage: #{$0} test-list TO FROM"
  puts "Example: #{$0} postfix/src/milter/test-list me@localhost me@example.com"
end

require 'English'

test_list, to, from, = ARGV

base = File.dirname(__FILE__) + "/"
title = nil
File.read(test_list).each_line do |line|
  line = line.chomp
  if /\A#/ =~ line
    title = $POSTMATCH.strip.downcase.gsub(/ tests\z/, '').gsub(/ /, "-")
    next
  end
  next if /\A\s*\z/ =~ line
  port = nil
  port = $1 if /inet:(\d+)@/ =~ line
  command = "connect"
  command = $1 if /-c (\w+)/ =~ line
  result = nil
  IO.popen("sudo tshark -a duration:3 -i any " +
           "-R 'tcp.port == #{port}' -n -T text -V") do |io|
    sleep(1)
    Dir.chdir(File.dirname(test_list)) do
      system("#{line} &")
      system("/usr/sbin/smtp-source -t #{to} -f #{from} localhost")
    end
    result = io.read
  end

  File.open("#{base}milter-packet-#{title}-on-#{command}.log", "w") do |log|
    log.puts(line)
    log.puts

    _, *data = result.split(/^(?=Data)/)
    n = 0
    data.each do |datum|
      datum = datum.sub(/\n\nFrame.*/m, '')
      if (n % 2).zero?
        log.puts datum
      else
        log.puts datum.gsub(/^/, '  ')
      end
      log.puts
      n += 1
    end
  end
end
