# Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

class TestChildContext < Test::Unit::TestCase
  include MilterEventLoopTestUtils

  def setup
    @clamav = Milter::Manager::Child.new("clamav-milter")
    @greylist = Milter::Manager::Child.new("milter-greylist")
    @spamass = Milter::Manager::Child.new("spamass-milter")

    @configuration = Milter::Manager::Configuration.new
    @loop = create_event_loop
    @children = Milter::Manager::Children.new(@configuration, @loop)
    @children << @clamav
    @children << @greylist
    @children << @spamass

    @context = create_context
  end

  def test_name
    assert_equal("clamav-milter", @context.name)
  end

  def test_macro
    @clamav.macro_context = Milter::COMMAND_CONNECT
    @clamav.set_macros(Milter::COMMAND_CONNECT,
                       {
                         "j" => "mail.example.com",
                         "daemon_name" => "mail.example.com",
                         "v" => "Postfix 2.5.5",
                       })
    assert_equal("mail.example.com", @context["daemon_name"])
    assert_equal("mail.example.com", @context["{daemon_name}"])
    assert_equal("Postfix 2.5.5", @context["v"])

    @clamav.macro_context = Milter::COMMAND_HELO
    @clamav.set_macros(Milter::COMMAND_HELO,
                       {
                         "v" => "Sendmail",
                       })
    assert_equal("Postfix 2.5.5", @context["v"])

    @context = create_context
    assert_equal("mail.example.com", @context["j"])
    assert_equal("Sendmail", @context["v"])
  end

  def test_set_macro
    @clamav.macro_context = Milter::COMMAND_CONNECT
    assert_equal(nil, @context["j"])
    @context["j"] = "mail.example.com"
    assert_equal("mail.example.com", @context["j"])

    @context = create_context
    assert_equal("mail.example.com", @context["j"])
  end

  def test_status
    @clamav.status = Milter::STATUS_REJECT
    assert_true(@context.reject?)
    @clamav.status = Milter::STATUS_DISCARD
    assert_true(@context.discard?)
    @clamav.status = Milter::STATUS_TEMPORARY_FAILURE
    assert_true(@context.temporary_failure?)
    @clamav.status = Milter::STATUS_ACCEPT
    assert_true(@context.accept?)
  end

  def test_children
    assert_equal("milter-greylist", @context.children["milter-greylist"].name)
  end

  def test_quitted?
    assert_predicate(@context, :quitted?)
  end

  def test_postfix?
    @clamav.macro_context = Milter::COMMAND_CONNECT
    assert_false(create_context.postfix?)

    @clamav.set_macros(Milter::COMMAND_CONNECT, "v" => "Sendmail")
    assert_false(create_context.postfix?)

    @clamav.set_macros(Milter::COMMAND_CONNECT, "v" => "Postfix 2.5.5")
    assert_true(create_context.postfix?)
  end

  def test_authenticated?
    @clamav.macro_context = Milter::COMMAND_ENVELOPE_FROM
    assert_false(create_context.authenticated?)

    @clamav.set_macros(Milter::COMMAND_ENVELOPE_FROM, "auth_type" => "CRAM-MD5")
    assert_true(create_context.authenticated?)

    @clamav.set_macros(Milter::COMMAND_ENVELOPE_FROM, "auth_type" => nil)
    assert_false(create_context.authenticated?)

    @clamav.set_macros(Milter::COMMAND_ENVELOPE_FROM, "auth_authen" => "user")
    assert_true(create_context.authenticated?)
  end

  private
  def create_context
    Milter::Manager::ChildContext.new(@clamav, @children, nil)
  end
end
