require 'test_helper'

class WelcomeControllerTest < ActionController::TestCase
  def test_menu_before_connection_setup
    default_connection = Config::Connection.default
    default_connection.spec = nil
    default_connection.save(false)
    get :index
    assert_select("ul.header-menu a[href=#{control_path}]", 0)
  end

  def test_menu_after_connection_setup
    get :index
    assert_select("ul.header-menu a[href=#{control_path}]")
  end
end
