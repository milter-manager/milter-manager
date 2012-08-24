#!/usr/bin/env ruby

require 'pathname'
require 'fileutils'
require 'optparse'

def main
  sf_user = nil
  parser = OptionParser.new
  parser.on("--sf-user=USER", "Update and upload to sf.net") do |user|
    sf_user = user
  end

  begin
    parser.parse!
  rescue OptionParser::ParseError => ex
    $stderr.puts ex.message
    $stderr.puts parser.help
    exit 1
  end

  tdiary_dir_map = setup_tdiary_dirs(Pathname("~/work/ruby/tdiary").expand_path)

  update(tdiary_dir_map)

  if sf_user
    run("./upload.rb", "--sf-user=#{sf_user}", tdiary_dir_map[:tdiary_compiled_dir].to_s)
  end
end

def setup_tdiary_dirs(tdiary_base_dir)
  tdiary_dir = tdiary_base_dir
  tdiary_clear_code_public_dir = tdiary_base_dir + "clear-code" + "public"
  blog_base_dir = nil
  tdiary_conf_dir = nil
  tdiary_conf = nil
  ["~/public_html/milter-manager"].each do |dir|
    conf = "#{dir}/blog/ja/tdiary.conf"
    conf = Pathname(conf).expand_path
    if conf.exist?
      blog_base_dir = Pathname(dir).expand_path
      tdiary_conf = conf
      tdiary_conf_dir = conf.dirname
      break
    end
  end
  tdiary_compiled_dir = blog_base_dir + "blog-html/ja"

  data_path_line = tdiary_conf.read.lines.grep(/^@data_path =/).join
  tdiary_data_path = eval(data_path_line)
  {
    :tdiary_dir                   => tdiary_dir,
    :tdiary_clear_code_public_dir => tdiary_clear_code_public_dir,
    :tdiary_conf_dir              => tdiary_conf_dir,
    :tdiary_compiled_dir          => tdiary_compiled_dir,
    :tdiary_data_path             => tdiary_data_path,
  }
end

def update(hash)
  tdiary_clear_code_public_dir = hash[:tdiary_clear_code_public_dir]
  tdiary_compiled_dir          = hash[:tdiary_compiled_dir]
  tdiary_dir                   = hash[:tdiary_dir]
  tdiary_conf_dir              = hash[:tdiary_conf_dir]
  Dir.chdir(tdiary_clear_code_public_dir.to_s) do
    FileUtils.rm_rf(tdiary_compiled_dir.to_s)
    run("./html-archiver.rb",
        "-t", tdiary_dir.to_s, "-c", tdiary_conf_dir.to_s,
        tdiary_compiled_dir.to_s)
  end
end

def run(*args)
  puts "[#{Dir.pwd}] #{args.join(' ')}"
  system(*args) or exit(1)
end

main

