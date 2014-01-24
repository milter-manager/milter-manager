#! /usr/bin/env ruby

require "json"
require "optparse"
require "net/https"

# URI: https://coveralls.io/api/v1/jobs
HOST = "coveralls.io"
PATH = "/api/v1/jobs"

def main
  info_filename = nil
  dump = false
  parser = OptionParser.new
  parser.on("--dump", "Dump JSON payload to STDOUT") do
    dump = true
  end
  parser.on("-h", "--help", "Print this message and quit") do
    puts parser.help
    exit(true)
  end
  parser.order!(ARGV)
  if ARGV.empty?
    puts parser.help
    exit(false)
  end

  info_filename = ARGV.shift
  lcov_info = parse_lcov_info(info_filename)

  source_files = []
  lcov_info.each do |filename, info|
    source_files << generate_source_file(filename, info)
  end

  # Add repo_token and server_job_id to run from local machine
  payload = {
    service_name: "travis-ci",
    service_job_id: ENV["TRAVIS_JOB_ID"],
    git: {
      head: {
        id: `git log --format=%H`,
        committer_email: `git log --format=%ce`,
        committer_name: `git log --format=%cN`,
        author_email: `git log --format=%ae`,
        author_name: `git log --format=%aN`,
        message: `git log --format=%s`,
      },
      remotes: [], # FIXME
      branch: `git rev-parse --abbrev-ref HEAD`,
    },
    source_files: source_files,
  }.to_json

  puts payload if dump
  post(payload)
end

def parse_lcov_info(filename)
  lcov_info = Hash.new{|h,k| h[k] = {} }
  source_file = nil
  File.readlines(filename).each do |line|
    case line.chomp
    when /\ASF:(.+)/
      source_file = $1
    when /\ADA:(\d+),(\d+)/
      line_no = $1.to_i
      count = $2.to_i
      lcov_info[source_file][line_no] = count
    when /\Aend_of_record/
      source_file = nil
    end
  end
  lcov_info
end

def generate_source_file(filename, info)
  source = File.read(filename)
  lines = source.lines
  coverage = Array.new(lines.to_a.size)
  source.lines.each_with_index do |line, index|
    coverage[index] = info[index + 1]
  end
  top_src_dir = Dir.pwd
  {
    name: filename.sub(%r!#{top_src_dir}/!, ""),
    source: source,
    coverage: coverage,
  }
end

def post(payload)
  Net::HTTP.version_1_2

  http = Net::HTTP.new(HOST, 443)
  http.use_ssl = true
  http.start do
    request = Net::HTTP::Post.new(PATH)
    request["content-type"] = "multipart/form-data; boundary=boundary"
    request.body = <<BODY.gsub(/\n/, "\r\n")
--boundary
content-disposition: form-data; name="json_file"; filename="payload.json"

#{payload}

--boundary--
BODY
    response = http.request(request)
    p response
    puts response.body
  end
end

main
