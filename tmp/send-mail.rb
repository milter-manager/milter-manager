#!/usr/bin/env ruby

require "net/smtp"

Net::SMTP.start("localhost") do |smtp|
  smtp.send_message(<<-EOM, "kou@localhost", "kou@localhost")
hayamiAZ
EOM
end
