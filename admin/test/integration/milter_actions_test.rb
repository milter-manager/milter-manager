require 'test_helper'

class MilterActionsTest < ActionController::IntegrationTest
  def test_create
    login

    assert_difference("Config::Milter.count", 1) do
      visit(config_milters_path)
      click_link(t("action.create"))
      fill_in(t("label.name"), :with => "milter1")
      select("inet", :from => t("label.type"))
      fill_in(t("label.port"), :with => "10025")
      fill_in(t("label.host"), :with => "localhost")
      click_button(t("action.create"))
    end
    assert_not_nil(Config::Milter.find_by_name("milter1"))
  end

  def test_update
    login

    anti_spam = milters(:anti_spam)
    visit(config_milters_path)
    click_link(anti_spam.name)
    assert_select("h2", "#{t('title.milter')} - #{anti_spam.name}")
    click_link(t("action.edit"))
    select("unix", :from => t("label.type"))
    fill_in(t("label.path"), :with => "/tmp/socket")
    fill_in(t("label.description"), :with => "An anti-spam milter")
    click_button(t("action.update"))

    expected_spec = "unix:/tmp/socket"
    expected_description = "An anti-spam milter"

    assert_not_equal(expected_spec, anti_spam.connection_spec)
    assert_not_equal(expected_description, anti_spam.description)
    anti_spam = ::Config::Milter.find(anti_spam)
    assert_equal(expected_spec, anti_spam.connection_spec)
    assert_equal(expected_description, anti_spam.description)
  end
end
