#!/usr/bin/env ruby

require "net/smtp"

Net::SMTP.start("localhost") do |smtp|
  smtp.send_message(<<-EOM, "haya@localhost", "haya@localhost")
hayamiAZ
EOM
end
