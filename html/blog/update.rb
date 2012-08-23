#!/usr/bin/env ruby

require 'pathname'
require 'fileutils'

if ARGV.delete("-h") or ARGV.delete("--help")
  puts "Usage: #{$0} [--upload]"
  puts "  e.g.: #{$0}          # only update"
  puts "      : #{$0} --upload # update and upload"
  exit
end

upload = ARGV[0] == "--upload"

def run(*args)
  puts "[#{Dir.pwd}] #{args.join(' ')}"
  system(*args) or exit(1)
end

tdiary_base_dir = Pathname("~/work/ruby/tdiary").expand_path
tdiary_dir = tdiary_base_dir + "core"
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

data_path_line = tdiary_conf.read.grep(/^@data_path =/).join
tdiary_data_path = eval(data_path_line)

milter_manager_doc_dir = Pathname("~/work/c/milter-manager-doc").expand_path

Dir.chdir(tdiary_data_path.to_s) do
  run("git", "pull", "--rebase")
end

Dir.chdir(tdiary_clear_code_public_dir.to_s) do
  run("svn", "up")
  FileUtils.rm_rf(tdiary_compiled_dir.to_s)
  run("./html-archiver.rb",
      "-t", tdiary_dir.to_s, "-c", tdiary_conf_dir.to_s,
      tdiary_compiled_dir.to_s)
end

if upload
  Dir.chdir(milter_manager_doc_dir.to_s) do
    run("git", "pull", "--rebase")
    run("./upload.rb", tdiary_compiled_dir.to_s)
  end
end
