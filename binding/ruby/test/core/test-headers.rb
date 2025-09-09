class TestHeader < Test::Unit::TestCase
  include MilterTestUtils

  def test_new
    header = Milter::Header.new("name", "value")
    assert_equal(["name", "value"], [header.name, header.value])
  end

  def test_compare
    header1 = Milter::Header.new("name1", "value1")
    header2 = Milter::Header.new("name2", "value2")
    assert_equal(0, header1 <=> header1)
    assert do
      (header1 <=> header2) < 0
    end
    assert do
      (header2 <=> header1) > 0
    end
  end

  def test_equal
    header1 = Milter::Header.new("name", "value")
    header2 = Milter::Header.new("name", "value")
    assert_true(header1 == header2)
  end

  def test_not_equal
    header1 = Milter::Header.new("name1", "value1")
    header2 = Milter::Header.new("name2", "value2")
    assert_false(header1 == header2)
  end
end

class TestHeaders < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @headers = Milter::Headers.new
  end

  def test_add
    expected_headers = [
      Milter::Header.new("name", "value")
    ]
    @headers.add("name", "value")
    assert_equal(1, @headers.size)
    assert_equal(expected_headers, @headers.to_a)
  end

  def test_add_same_name
    expected_headers = [
      Milter::Header.new("name", "value2"),
      Milter::Header.new("name", "value1"),
    ]
    @headers.add("name", "value1")
    @headers.add("name", "value2")
    assert_equal(2, @headers.size)
    assert_equal(expected_headers, @headers.to_a)
  end

  def test_append
    expected_headers = [
      Milter::Header.new("name1", "value1"),
      Milter::Header.new("name2", "value2"),
    ]
    @headers.append("name1", "value1")
    @headers.append("name2", "value2")
    assert_equal(2, @headers.size)
    assert_equal(expected_headers, @headers.to_a)
  end

  def test_append_same_name
    expected_headers = [
      Milter::Header.new("name", "value1"),
      Milter::Header.new("name", "value2"),
    ]
    @headers.append("name", "value1")
    @headers.append("name", "value2")
    assert_equal(2, @headers.size)
    assert_equal(expected_headers, @headers.to_a)
  end

  def test_insert
    expected_headers = [
      Milter::Header.new("name1", "value1"),
      Milter::Header.new("name2", "value2"),
      Milter::Header.new("name3", "value3"),
      Milter::Header.new("name4", "value4"),
    ]
    @headers.append("name1", "value1")
    @headers.append("name3", "value3")
    @headers.append("name4", "value4")
    @headers.insert(1, "name2", "value2")
    assert_equal(4, @headers.size)
    assert_equal(expected_headers, @headers.to_a)
  end

  def test_not_found
    assert_nil(@headers.find("name", "value"))
  end

  def test_find
    @headers.add("name1", "value1")
    @headers.add("name2", "value2")
    @headers.add("name3", "value3")
    assert_equal(3, @headers.size)
    header = @headers.find("name2", "value2")
    assert_equal(["name2", "value2"], [header.name, header.value])
  end

  def test_get_nth_out_of_range
    assert_nil(@headers.get_nth(0))
    assert_nil(@headers.get_nth(1))
  end

  def test_get_nth
    @headers.add("name1", "value1")
    @headers.add("name2", "value2")
    @headers.add("name3", "value3")
    assert_equal(3, @headers.size)
    header = @headers.get_nth(2)
    assert_equal(["name2", "value2"], [header.name, header.value])
  end

  def test_remove
    assert_false(@headers.remove("name1", "value1"))
    @headers.add("name1", "value1")
    assert_equal(1, @headers.size)
    assert_true(@headers.remove("name1", "value1"))
    assert_equal([], @headers.to_a)
    assert_false(@headers.remove("name1", "value1"))
  end

  def test_delete
    expected_headers = [
      Milter::Header.new("name", "value1"),
      Milter::Header.new("name2", "value2"),
    ]
    @headers.append("name", "value1")
    @headers.append("name2", "value2")
    @headers.append("name", "value3")
    @headers.delete("name", 2)
    assert_equal(expected_headers, @headers.to_a)
  end

  def test_change
    expected_headers = [
      Milter::Header.new("name", "value1"),
      Milter::Header.new("name2", "value2"),
      Milter::Header.new("name", "changed value3"),
    ]
    @headers.append("name", "value1")
    @headers.append("name2", "value2")
    @headers.append("name", "value3")
    @headers.change("name", 2, "changed value3")
    assert_equal(expected_headers, @headers.to_a)
  end

  def test_dup
    expected_headers = [
      Milter::Header.new("name1", "value1"),
      Milter::Header.new("name2", "value2"),
      Milter::Header.new("name3", "value3"),
    ]
    headers = Milter::Headers.new
    headers.append("name1", "value1")
    headers.append("name2", "value2")
    headers.append("name3", "value3")
    copied_headers = headers.dup
    assert_equal(expected_headers, copied_headers.to_a)
  end
end
