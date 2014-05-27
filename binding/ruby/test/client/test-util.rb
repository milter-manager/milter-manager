#-*- coding: utf-8 -*-

require "milter/client/util"

class TestUtil < Test::Unit::TestCase
  include Milter::Client::Util

  class AddressSpec < self
    data("with angle bracket"  => ["foo@example.net", "<foo@example.net>"],
         "address only"        => ["foo@example.net", "foo@example.net"],
         "upper case letters"  => ["foo@example.net", "FOO@EXAMPLE.NET"],
         "empty angle bracket" => ["", "<>"],
         "empty"               => ["", ""],
         "unbalanced <foo"     => ["foo", "<foo"],
         "unbalanced foo>"     => ["foo", "foo>"])
    def test_address_spec(data)
      expected, target = data
      assert_equal(expected, address_spec(target))
    end

    def test_nil
      assert_nil(address_spec(nil))
    end

    data("UTF-8"       => "UTF-8",
         "ISO-2022-JP" => "ISO-2022-JP",
         "Shift_JIS"   => "Shift_JIS",
         "EUC-JP"      => "EUC-JP")
    def test_encoding(encoding)
      address = %Q!日本語 <bob@example.com>!
      encoded_address = address.encode(encoding).force_encoding("UTF-8")
      assert_equal("bob@example.com", address_spec(encoded_address))
    end
  end

  class DomainName < self
    def test_nil
      assert_nil(domain_name(nil))
    end

    def test_empty
      assert_nil(domain_name(""))
    end

    def test_plain
      assert_equal("example.com", domain_name("foo@example.com"))
    end

    def test_local_part
      assert_nil(domain_name("foo"))
    end
  end
end
