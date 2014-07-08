#-*- coding: utf-8 -*-

require "milter/client/envelope-address"

class TestUtil < Test::Unit::TestCase
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
      envelope_address = Milter::Client::EnvelopeAddress.new(target)
      assert_equal(expected, envelope_address.address_spec)
    end

    def test_nil
      envelope_address = Milter::Client::EnvelopeAddress.new(nil)
      assert_nil(envelope_address.address_spec)
    end

    data("UTF-8"       => "UTF-8",
         "ISO-2022-JP" => "ISO-2022-JP",
         "Shift_JIS"   => "Shift_JIS",
         "EUC-JP"      => "EUC-JP")
    def test_encoding(encoding)
      address = %Q!日本語 <bob@example.com>!
      encoded_address = address.encode(encoding).force_encoding("UTF-8")
      envelope_address = Milter::Client::EnvelopeAddress.new(encoded_address)
      assert_equal("bob@example.com", envelope_address.address_spec)
    end
  end

  class DomainName < self
    def test_nil
      envelope_address = Milter::Client::EnvelopeAddress.new(nil)
      assert_nil(envelope_address.domain)
    end

    def test_empty
      envelope_address = Milter::Client::EnvelopeAddress.new("")
      assert_nil(envelope_address.domain)
    end

    def test_plain
      envelope_address = Milter::Client::EnvelopeAddress.new("foo@example.com")
      assert_equal("example.com", envelope_address.domain)
    end

    def test_local_part
      envelope_address = Milter::Client::EnvelopeAddress.new("foo")
      assert_nil(envelope_address.domain)
    end
  end
end
