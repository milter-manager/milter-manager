# Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

require 'shellwords'
begin
  require 'pkg-config'
rescue LoadError
end

module Milter
  class TestServer
    attr_reader :options
    def initialize
      @milter_test_server = guess_milter_test_server_path
      @options = []
    end

    def run(*options)
      if options.size == 1 and options.first.is_a?(Hash)
        options = options.first.collect do |key, values|
          [key, values]
        end
      end
      command_line = [@milter_test_server,
                      "--output-message",
                      "--color", "no"]
      (@options + options).each do |key, values|
        key = key.to_s.gsub(/_/, "-").gsub(/\A--/, '')
        values = [values] unless values.is_a?(Array)
        values.each do |value|
          command_line.concat(["--#{key}", value])
        end
      end
      escaped_command_line = Shellwords.join(command_line)
      IO.popen(escaped_command_line, 'r') do |command_result|
        parse(command_result)
      end
    end

    private
    def guess_milter_test_server_path
      path = ENV["MILTER_TEST_SERVER"]
      if defined?(::PKGConfig)
        path ||= PKGConfig.variable("milter-server",
                                    "milter_test_server")
      end
      path || "milter-test-server"
    end

    def parse(command_result)
      if Object.const_defined?(:Encoding)
        command_result.set_encoding(Encoding::ASCII_8BIT)
      end
      result = TestServerResult.new

      result.status = command_result.gets.chomp.split(/: /, 2)[1]
      result.elapsed_time = command_result.gets.split(/: /, 2)[1].split[0]
      command_result.gets

      mode = nil
      while line = command_result.gets
        case line
        when /\AEnvelope:/
          mode = :envelope
        when /\AHeader:/
          mode = :headers
        when /\ABody:/
          break
        else
          case mode
          when :envelope
            next if line[0, 1] == "-"
            target, value = line[1..-1].strip.split(/:/, 2)
            case target
            when "MAIL FROM"
              result.envelope_from = value
            when "RCPT TO"
              result.envelope_recipients << value
            end
          when :headers
            name, value = line.chomp.lstrip.split(/:\s*/, 2)
            result.headers << [name, value]
          end
        end
      end
      result.body = command_result.read

      result
    end
  end

  class TestServerResult
    attr_accessor :status, :elapsed_time
    attr_accessor :envelope_from, :envelope_recipients, :headers, :body
    def initialize
      @status = nil
      @elapsed_time = nil
      @envelope_from = nil
      @envelope_recipients = []
      @headers = []
      @body = nil
    end

    def to_hash
      {
        :status => @status,
        :elapsed_time => @elapsed_time,
        :envelope_from => @envelope_from,
        :envelope_recipients => @envelope_recipients,
        :headers => @headers,
        :body => @body,
      }
    end
  end
end
