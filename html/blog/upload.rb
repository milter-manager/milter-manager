#!/usr/bin/env ruby

require 'net/ftp'
require 'pathname'
require 'optparse'

def main
  osdn_user = nil
  parser = OptionParser.new
  parser.on("--osdn-user=[USER]", "Specify sf.net user") do |user|
    osdn_user = user
  end

  begin
    parser.parse!
  rescue OptionParser::ParseError => ex
    $stderr.puts ex.message
    $stderr.puts parser.help
    exit 1
  end

  tdiary_compiled_dir = ARGV.shift
  if tdiary_compiled_dir.nil?
    tdiary_conf_dir = nil
    ["~/public_html/milter-manager"].each do |dir|
      conf = "#{dir}/blog/ja/tdiary.conf"
      conf = Pathname(conf).expand_path
      if conf.exist?
        blog_base_dir = Pathname(dir).expand_path
        tdiary_conf_dir = conf.dirname
        break
      end
    end
    tdiary_compiled_dir = (blog_base_dir + "blog-html/ja").to_s
  end

  osdn_project = "milter-manager"
  osdn_host = "shell.osdn.net"
  osdn_htdocs_path = "/home/groups/#{osdn_project[0]}/#{osdn_project[0..1]}/#{osdn_project}/htdocs"
  system("rsync", "-avz", "--delete", tdiary_compiled_dir,
         "#{osdn_user}@#{osdn_host}:#{osdn_htdocs_path}/blog/")
end

main
