#!/usr/bin/env ruby

require 'optparse'

target_language = nil
opts = OptionParser.new do |opts|
  opts.on("--help", "Show this message") do
    puts opts
    exit 0
  end

  opts.on("--language=LANGUAGE", "Target language") do |language|
    target_language = language
  end
end
opts.parse!

def read_template(type, language)
  base_dir = File.dirname(__FILE__)
  file = [type, language].compact.join("-") + ".html"
  File.read(File.join(base_dir, file))
end

head = read_template("head", target_language)
header = read_template("header", target_language)
footer = read_template("footer", target_language)
ARGV.each do |target|
  File.open(target, "r+") do |input|
    content = input.read
    content = content.sub(/(<\/title>)/, " - milter manager\\1")
    content = content.sub(/(<\/head>)/, head + "\n\\1")
    content = content.sub(/(<body\s.+?>)/, "\\1\n" + header)
    content = content.sub(/(<\/body>)/, footer + "\n\\1")
    input.seek(0)
    input.print(content)
  end
end
