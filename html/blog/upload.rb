#!/usr/bin/env ruby

require 'net/ftp'
require 'pathname'
require 'optparse'

def main
  sf_user = nil
  parser = OptionParser.new
  parser.on("--sf-user=[USER]", "Specify sf.net user") do |user|
    sf_user = user
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

  sf_project = "milter-manager"
  sf_host = "web.sourceforge.net"
  sf_htdocs_path = "/home/groups/m/mi/milter-manager/htdocs"
  system("rsync", "-avz", "--delete", tdiary_compiled_dir,
         "#{sf_user},#{sf_project}@#{sf_host}:#{sf_htdocs_path}/blog/")
end

main
