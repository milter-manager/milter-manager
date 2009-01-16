require 'test_helper'

class ConnectionActionsTest < ActionController::IntegrationTest
  def test_update
    visit(config_path)
    click_link(t("menu.config-connection"))
    assert_select("h2", t('title.connection'))
    click_link(t("action.edit"))
    selects("inet", :from => t("label.type"))
    fill_in(t("label.port"), :with => "2929")
    fill_in(t("label.host"), :with => "localhost")
    click_button(t("action.update"))
    assert_equal("inet:2929@localhost", Config::Connection.default.spec)
  end
end
