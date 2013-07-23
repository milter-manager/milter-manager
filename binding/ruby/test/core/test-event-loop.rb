# Copyright (C) 2011-2013  Kouhei Sutou <kou@clear-code.com>
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

class TestEventLoop < Test::Unit::TestCase
  include MilterTestUtils
  include MilterEventLoopTestUtils

  def setup
    @loop = create_event_loop
    @tags = []
  end

  def teardown
    @tags.each do |tag|
      @loop.remove(tag)
    end
  end

  def test_timeout
    timeouted = false
    interval = 0.001
    @tags << @loop.add_timeout(interval) do
      timeouted = true
      false
    end
    sleep(interval)
    assert_true(@loop.iterate(:may_block => false))
    assert_true(timeouted)
  end

  def test_timeout_not_timeouted
    timeouted = false
    interval = 1
    @tags << @loop.add_timeout(interval) do
      timeouted = true
      false
    end
    assert_false(@loop.iterate(:may_block => false))
    assert_false(timeouted)
  end

  def test_timeout_without_block
    assert_raise(ArgumentError.new("timeout block is missing")) do
      @tags << @loop.add_timeout(1)
    end
  end

  def test_timeout_exception
    n_called_before = n_called_after = 0
    interval = 0.001
    @tags << @loop.add_timeout(interval) do
      n_called_before += 1
      raise "should be rescued"
      n_called_after += 1
    end
    sleep(interval)
    assert_nothing_raised {@loop.iterate(:may_block => false)}
    assert_equal([1, 0], [n_called_before, n_called_after])
    sleep(interval)
    assert_nothing_raised {@loop.iterate(:may_block => false)}
    assert_equal([1, 0], [n_called_before, n_called_after])
  end

  def test_idle
    idled = false
    @tags << @loop.add_idle do
      idled = true
      false
    end
    assert_true(@loop.iterate(:may_block => false))
    assert_true(idled)
  end

  def test_idle_not_idled
    idled = false
    @tags << @loop.add_idle do
      idled = true
      false
    end
    @tags << @loop.add_timeout(0.000001) do
      false
    end
    assert_true(@loop.iterate(:may_block => false))
    assert_false(idled)
  end

  def test_idle_without_block
    assert_raise(ArgumentError.new("idle block is missing")) do
      @tags << @loop.add_idle
    end
  end

  def test_idle_exception
    n_called_before = n_called_after = 0
    @tags << @loop.add_idle do
      n_called_before += 1
      raise "should be rescued"
      n_called_after += 1
    end
    assert_nothing_raised {@loop.iterate(:may_block => false)}
    assert_equal([1, 0], [n_called_before, n_called_after])
    assert_nothing_raised {@loop.iterate(:may_block => false)}
    assert_equal([1, 0], [n_called_before, n_called_after])
  end

  def test_watch_child
    callback_arguments = nil
    pid = fork do
      exit!(true)
    end
    @tags << @loop.watch_child(pid) do |*args|
      callback_arguments = args
      false
    end
    sleep(0.01)
    assert_true(@loop.iterate(:may_block => false))
    assert_equal(1, callback_arguments.size)
    status = callback_arguments[0]
    assert_equal([pid, true, true],
                 [status.pid, status.exited?, status.success?])
  end

  def test_watch_child_not_reaped
    callback_arguments = nil
    pid = fork do
      sleep(0.1)
      exit!(true)
    end
    @tags << @loop.watch_child(pid) do |*args|
      callback_arguments = args
      false
    end
    assert_false(@loop.iterate(:may_block => false))
    assert_nil(callback_arguments)
  end

  def test_watch_child_without_block
    pid = fork do
      exit!(true)
    end
    assert_raise(ArgumentError.new("watch child block is missing")) do
      @tags << @loop.watch_child(pid)
    end
  ensure
    Process.wait(pid)
  end

  def test_watch_child_exception
    n_called_before = n_called_after = 0
    pid = fork do
      exit!(true)
    end
    @tags << @loop.watch_child(pid) do |*args|
      n_called_before += 1
      raise "should be rescued"
      n_called_after += 1
    end
    sleep(0.01)
    assert_nothing_raised {@loop.iterate(:may_block => false)}
    assert_equal([1, 0], [n_called_before, n_called_after])
  end

  def test_watch_io
    callback_arguments = nil
    read_data = nil
    parent_read, child_write = IO.pipe
    flush_notify_read, flush_notify_write = IO.pipe
    pid = fork do
      child_write.puts("child")
      child_write.flush
      flush_notify_write.puts("flushed")
      flush_notify_write.flush
      sleep(0.1)
      exit!(true)
    end
    input = GLib::IOChannel.new(parent_read)
    @tags << @loop.watch_io(input, GLib::IOChannel::IN) do |channel, condition|
      callback_arguments = [channel.class, condition]
      read_data = channel.readline
      false
    end
    flush_notify_read.gets
    assert_true(@loop.iterate(:may_block => false))
    assert_equal([[input.class, GLib::IOChannel::IN], "child\n"],
                 [callback_arguments, read_data])
  end

  def test_watch_io_without_block
    parent_read, child_write = IO.pipe
    pid = fork do
      child_write.puts("child")
      exit!(true)
    end
    input = GLib::IOChannel.new(parent_read)
    assert_raise(ArgumentError.new("watch IO block is missing")) do
      @tags << @loop.watch_io(input, GLib::IOChannel::IN)
    end
  end

  def test_watch_io_exception
    n_called_before = n_called_after = 0
    sleep_time = 0.01
    parent_read, child_write = IO.pipe
    pid = fork do
      child_write.puts("child")
      sleep(sleep_time)
      child_write.puts("child")
      exit!(true)
    end
    input = GLib::IOChannel.new(parent_read)
    @tags << @loop.watch_io(input, GLib::IOChannel::IN) do |channel, condition|
      n_called_before += 1
      raise "should be rescued"
      n_called_after += 1
    end
    sleep(sleep_time)
    assert_nothing_raised {@loop.iterate(:may_block => false)}
    assert_equal([1, 0], [n_called_before, n_called_after])
    assert_nothing_raised {@loop.iterate(:may_block => false)}
    assert_equal([1, 0], [n_called_before, n_called_after])
  end

  def test_run
    n_called = 0
    quitted = false
    @tags << @loop.add_idle do
      n_called += 1
      if n_called == 3
        @loop.quit
        false
      else
        true
      end
    end
    @loop.run
    assert_equal(3, n_called)
  end
end
