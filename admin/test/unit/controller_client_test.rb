require 'test_helper'

class ControllerClientTest < ActiveSupport::TestCase
  def test_status
    TCPServer.open("127.0.0.1", 0) do |server|
      port = server.addr[1]
      Thread.new do
        _client = server.accept
        _client.print(packet("status", "XXX"))
      end
      assert_equal("XXX", client("inet:#{port}").status)
    end
  end

  def test_configuration
      now = Time.now.to_i
      configuration = <<-EOC
<configuration>
  <last-modified>#{now}</last-modified>
  <milters>
    <milter>
      <name>milter@10025</name>
      <connection-spec>inet:10025</connection-spec>
      <command>test-milter</command>
    </milter>
  </milters>
</configuration>
EOC
    assert_configuration({"last_modified" => Time.at(now),
                           "milters" => [
                                         {
                                           "name" => "milter@10025",
                                           "connection_spec" => "inet:10025",
                                           "command" => "test-milter",
                                         },
                                        ]},
                         configuration)

      configuration = <<-EOC
<configuration>
  <milters>
    <milter>
      <name>milter@10025</name>
      <connection-spec>inet:10025</connection-spec>
      <command>test-milter</command>
      <applicable-conditions>
        <applicable-condition>S25R</applicable-condition>
      </applicable-conditions>
    </milter>
  </milters>
  <applicable-conditions>
    <applicable-condition>
      <name>S25R</name>
      <description>Selective SMTP Rejection.</description>
    </applicable-condition>
    <applicable-condition>
      <name>remote-network</name>
      <description>Check only remote network.</description>
    </applicable-condition>
  </applicable-conditions>
</configuration>
EOC
    assert_configuration({
                           "milters" =>
                           [
                            {
                              "name" => "milter@10025",
                              "connection_spec" => "inet:10025",
                              "command" => "test-milter",
                              "applicable_conditions" => ["S25R"],
                            },
                           ],
                           "applicable_conditions" =>
                           [
                            {
                              "name" => "S25R",
                              "description" => "Selective SMTP Rejection.",
                            },
                            {
                              "name" => "remote-network",
                              "description" => "Check only remote network.",
                            }
                           ]
                         },
                         configuration)
  end

  def test_parse_success
    assert_equal("message",
                 client.send(:parse, "success", "message"))
  end

  def test_parse_error
    error = Milter::Manager::ControllerClient::Error.new("message")
    assert_raise(error) do
      client.send(:parse, "error", "message")
    end
  end

  private
  def client(spec=nil)
    Milter::Manager::ControllerClient.new(spec || "inet:2929")
  end

  def packet(command, content='')
    pack(command + "\0" + content)
  end

  def pack(content)
    [content.size].pack("N") + content
  end

  def assert_configuration(expected, config_xml)
    thread = nil
    TCPServer.open("127.0.0.1", 12929) do |server|
      thread = Thread.new do
        _client = server.accept
        _client.print(packet("configuration", config_xml))
      end
      assert_equal(expected, client("inet:12929").configuration)
    end
  ensure
    if thread
      thread.kill if thread.join(1).nil?
    end
  end
end
