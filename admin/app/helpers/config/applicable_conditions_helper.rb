module Config::ApplicableConditionsHelper
  def config_applicable_conditions_show_menus
    [
     content_tag("li", link_to(t("action.list"),
                               config_applicable_conditions_path)),
    ]
  end
end
