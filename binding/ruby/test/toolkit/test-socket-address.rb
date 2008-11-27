class TestSocketAddresss < Test::Unit::TestCase
  include MilterTestUtils

  def test_ipv4_to_s
    address = Milter::SocketAddress::IPv4.new("127.0.0.1", 2929)

    assert_equal("inet:2929@127.0.0.1", address.to_s)
  end

  def test_ipv4_equal
    address1 = Milter::SocketAddress::IPv4.new("127.0.0.1", 2929)
    address2 = Milter::SocketAddress::IPv4.new("127.0.0.1", 9292)
    address3 = Milter::SocketAddress::IPv4.new("127.0.0.1", 2929)

    assert_equal(address1, address1)
    assert_not_equal(address1, address2)
    assert_equal(address1, address1)
  end

  def test_ipv6_to_s
    address = Milter::SocketAddress::IPv6.new("::1", 2929)

    assert_equal("inet6:2929@::1", address.to_s)
  end

  def test_ipv6_equal
    address1 = Milter::SocketAddress::IPv6.new("::1", 2929)
    address2 = Milter::SocketAddress::IPv6.new("::1", 9292)
    address3 = Milter::SocketAddress::IPv6.new("::1", 2929)

    assert_equal(address1, address1)
    assert_not_equal(address1, address2)
    assert_equal(address1, address1)
  end

  def test_unix_to_s
    address = Milter::SocketAddress::Unix.new("/tmp/local.sock")

    assert_equal("unix:/tmp/local.sock", address.to_s)
  end

  def test_unix_equal
    address1 = Milter::SocketAddress::Unix.new("/tmp/local.sock")
    address2 = Milter::SocketAddress::Unix.new("/tmp/other.sock")
    address3 = Milter::SocketAddress::Unix.new("/tmp/local.sock")

    assert_equal(address1, address1)
    assert_not_equal(address1, address2)
    assert_equal(address1, address1)
  end
end
