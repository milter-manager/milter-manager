#!/usr/bin/env ruby

require 'pathname'
require 'fileutils'
require 'optparse'

def main
  osdn_user = nil
  milter_manager_source_path = "~/wc/milter-manager"
  tdiary_source_path = "~/work/ruby/tdiary"
  langs = ["en", "ja"]
  parser = OptionParser.new
  parser.on("--osdn-user=USER", "Update and upload to OSDN") do |user|
    osdn_user = user
  end
  parser.on("--milter-manager-source-path=PATH", "Path to milter-manager source") do |path|
    milter_manager_source_path = path
  end
  parser.on("--tdiary-path=PATH", "Path to tdiary source") do |path|
    tdiary_path = path
  end
  parser.on("--langs=LANGS", "Comma separated target languages ") do |languages|
    langs = languages.split(",")
  end

  begin
    parser.parse!
  rescue OptionParser::ParseError => ex
    $stderr.puts ex.message
    $stderr.puts parser.help
    exit 1
  end

  langs.each do |lang|
    tdiary_dir_map = setup_tdiary_dirs(Pathname(tdiary_source_path).expand_path, lang, milter_manager_source_path)

    update(tdiary_dir_map)

    if osdn_user
      run("./upload.rb", "--osdn-user=#{osdn_user}", tdiary_dir_map[:tdiary_compiled_dir].to_s)
    end
  end
end

def setup_tdiary_dirs(tdiary_base_dir, lang, milter_manager_source_path)
  tdiary_dir = tdiary_base_dir
  tdiary_clear_code_public_dir = tdiary_base_dir + "clear-code" + "public"
  blog_base_dir = nil
  tdiary_conf_dir = nil
  tdiary_conf = nil
  ["#{milter_manager_source_path}/html"].each do |dir|
    conf = case lang
           when "en"
             "#{dir}/blog/tdiary.en.conf"
           when "ja"
             "#{dir}/blog/tdiary.ja.conf"
           end
    conf = Pathname(conf).expand_path
    if conf.exist?
      blog_base_dir = Pathname(dir).expand_path
      tdiary_conf = conf
      tdiary_conf_dir = conf.dirname
      break
    end
  end
  tdiary_compiled_dir = blog_base_dir + "blog-html/#{lang}"

  dest_conf = File.join(tdiary_conf_dir, "tdiary.conf")
  FileUtils.cp(tdiary_conf, dest_conf)
  data_path_line = tdiary_conf.read.lines.grep(/^@data_path =/).join
  tdiary_data_path = eval(data_path_line)
  {
    :tdiary_dir                   => tdiary_dir,
    :tdiary_clear_code_public_dir => tdiary_clear_code_public_dir,
    :tdiary_conf_dir              => tdiary_conf_dir,
    :tdiary_compiled_dir          => tdiary_compiled_dir,
    :tdiary_data_path             => tdiary_data_path,
    :tdiary_conf                  => tdiary_conf
  }
end

def update(hash)
  tdiary_clear_code_public_dir = hash[:tdiary_clear_code_public_dir]
  tdiary_compiled_dir          = hash[:tdiary_compiled_dir]
  tdiary_dir                   = hash[:tdiary_dir]
  tdiary_conf_dir              = hash[:tdiary_conf_dir]
  tdiary_conf                  = hash[:tdiary_conf]
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

