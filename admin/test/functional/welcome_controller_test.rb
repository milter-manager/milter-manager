require 'test_helper'

class WelcomeControllerTest < ActionController::TestCase
  include AuthenticatedTestHelper

  def test_without_login
    get(:index)
    assert_redirected_to(new_session_path)
  end

  def test_menu_before_connection_setup
    login_as(:aaron)
    default_connection = Config::Connection.default
    default_connection.spec = nil
    default_connection.save(false)
    get(:index)
    assert_response(:success)
    assert_select("ul.header-menu a[href=#{control_path}]", 0)
  end

  def test_menu_after_connection_setup
    login_as(:aaron)
    get(:index)
    assert_response(:success)
    # TODO: enable control page.
    assert_select("ul.header-menu a[href=#{control_path}]", 0)
  end
end
