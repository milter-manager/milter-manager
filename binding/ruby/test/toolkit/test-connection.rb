class TestConnection < Test::Unit::TestCase
  include MilterTestUtils

  def test_parse_spec_ipv4
    assert_equal(Milter::SocketAddress::IPv4.new("127.0.0.1", 9999),
                 Milter::Connection.parse_spec("inet:9999@127.0.0.1"))
  end

  def test_parse_spec_ipv6
    assert_equal(Milter::SocketAddress::IPv6.new("::1", 9999),
                 Milter::Connection.parse_spec("inet6:9999@::1"))
  end

  def test_parse_spec_unix
    assert_equal(Milter::SocketAddress::Unix.new("/tmp/socket"),
                 Milter::Connection.parse_spec("unix:/tmp/socket"))
  end
end
