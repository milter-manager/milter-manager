require 'test_helper'

class ConnectionSpecTest < ActiveSupport::TestCase
  def test_parse
    assert_spec([:unix, {:path => "/tmp/milter-manager-admin.sock"}],
                "unix:/tmp/milter-manager-admin.sock")

    assert_spec([:unix, {:path => "/tmp/milter-manager-admin.sock"}],
                "local:/tmp/milter-manager-admin.sock")

    assert_spec([:inet, {:port => 2929, :host => "localhost"}],
                "inet:2929@localhost")
    assert_spec([:inet, {:port => 2929, :host => nil}],
                "inet:2929")

    assert_spec([:inet6, {:port => 2929, :host => "localhost"}],
                "inet6:2929@localhost")
    assert_spec([:inet6, {:port => 2929, :host => nil}],
                "inet6:2929")
  end

  def test_parse_fail
    assert_invalid_spec("separator colon is missing: <inet>",
                        "inet")
    assert_invalid_spec("invalid spec type: <unknown>: <unknown:>",
                        "unknown:")
    assert_invalid_spec("UNIX domain socket path is missing: <unix:>",
                        "unix:")
    assert_invalid_spec("port number is missing: <inet:>",
                        "inet:")
    assert_invalid_spec("port number is missing: <inet6:>",
                        "inet6:")
  end

  def test_open
    TCPServer.open("localhost", 0) do |server|
      port = server.addr[1]
      spec = parse("inet:#{port}@localhost")
      spec.open do |client|
        client.puts("text")
        assert_equal("text\n", server.accept.gets)
      end
    end
  end

  def test_decompose
    assert_equal(["unix", {:path => "/tmp/milter.sock"}],
                 build(:unix, :path => "/tmp/milter.sock").decompose)
    assert_equal(["inet", {:port => 2929}],
                 build(:inet, :port => 2929).decompose)
    assert_equal(["inet", {:port => 2929, :host => "example.com"}],
                 build(:inet, :port => 2929, :host => "example.com").decompose)
    assert_equal(["inet6", {:port => 2929}],
                 build(:inet6, :port => 2929).decompose)
    assert_equal(["inet6", {:port => 2929, :host => "example.com"}],
                 build(:inet6, :port => 2929, :host => "example.com").decompose)
  end

  def test_to_s
    assert_equal("unix:/tmp/milter.sock",
                 build(:unix, :path => "/tmp/milter.sock").to_s)
    assert_equal("inet:2929",
                 build(:inet, :port => 2929).to_s)
    assert_equal("inet:2929@example.com",
                 build(:inet, :port => 2929, :host => "example.com").to_s)
    assert_equal("inet6:2929",
                 build(:inet6, :port => 2929).to_s)
    assert_equal("inet6:2929@example.com",
                 build(:inet6, :port => 2929, :host => "example.com").to_s)
  end

  private
  def parse(spec)
    Milter::Manager::ConnectionSpec.parse(spec)
  end

  def build(type, options)
    Milter::Manager::ConnectionSpec.build(type, options)
  end

  def assert_spec(expected, spec)
    assert_equal(build(*expected), parse(spec))
  end

  def assert_invalid_spec(message, spec)
    assert_raise(ArgumentError.new(message)) do
      parse(spec)
    end
  end
end
