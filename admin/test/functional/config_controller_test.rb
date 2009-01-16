require 'test_helper'

class ConfigControllerTest < ActionController::TestCase
  def test_sub_menu_before_connection_setup
    default_connection = Config::Connection.default
    default_connection.spec = nil
    default_connection.save(false)
    get :index
    assert_select("div.sub-menu a[href=#{config_milters_path}]", 0)
    assert_select("div.sub-menu a[href=#{config_applicable_conditions_path}]", 0)
  end

  def test_sub_menu_after_connection_setup
    get :index
    assert_select("div.sub-menu a[href=#{config_milters_path}]")
    assert_select("div.sub-menu a[href=#{config_applicable_conditions_path}]")
  end
end
