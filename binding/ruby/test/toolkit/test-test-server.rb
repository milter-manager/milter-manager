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

class TestTestServer < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @test_server = Milter::TestServer.new
    @connection_spec = "inet:50025"
    @pid = nil
  end

  def teardown
    kill_process(@pid) if @pid
  end

  def test_nothing
    @pid = fork do
      exec(milter_test_client,
           "--connection-spec", @connection_spec,
           "--no-report-request")
      exit!(false)
    end
    10.times do
      begin
        TCPSocket.new("localhost", 50025)
        break
      rescue SystemCallError
        sleep(0.1)
      end
    end
    result = @test_server.run(:connection_spec => @connection_spec)
    result_hash = result.to_hash
    elapsed_time = result_hash.delete(:elapsed_time)
    assert_equal({
                   :status => "pass",
                   :envelope_from => "<sender@example.com>",
                   :envelope_recipients => ["<receiver@example.org>"],
                   :headers => [["From", "<sender@example.com>"],
                                ["To", "<receiver@example.org>"]],
                   :body => "La de da de da 1.\n" +
                            "La de da de da 2.\n" +
                            "La de da de da 3.\n" +
                            "La de da de da 4.\n",
                 },
                 result_hash)
    assert_match(/\A\d+\.\d+\z/, elapsed_time)
  end

  private
  def milter_test_client
    ENV["MILTER_TEST_CLIENT"] || "milter-test-client"
  end

  def kill_process(pid)
    Process.kill(:INT, pid)
    waiting_time = Time.now
    while Process.waitpid(pid, Process::WNOHANG).nil?
      sleep(0.1)
      return if (Time.now.to_i - waiting_time.to_i) >= 1
    end
    Process.kill(:KILL, pid)
    Process.waitpid(pid)
  end
end
