module Config::MiltersHelper
  def milter_target_type_select_tag(name, type)
    target_types = Config::Milter.target_types.collect do |_type|
      [t("label.target_#{_type}"), _type]
    end
    select_tag(name, options_for_select(target_types, type))
  end

  def format_milter_target_value(type, value)
    if value.is_a?(Array)
      value.collect do |val|
        format_milter_target_value(type, val)
      end.join('=')
    else
      case value
      when Regexp
        value = value.inspect
      end
      h(value)
    end
  end

  def config_milters_index_menus
    [content_tag("li", link_to(t("action.create"), new_config_milter_path))]
  end

  def config_milters_new_menus
    [
     content_tag("li", link_to(t("action.list"), config_milters_path)),
    ]
  end

  def config_milters_show_menus
    [
     content_tag("li", link_to(t("action.list"), config_milters_path)),
     content_tag("li", link_to(t("action.edit"), edit_config_milter_path)),
     content_tag("li", link_to(t("action.create"), new_config_milter_path))
    ]
  end

  def config_milters_edit_menus
    [
     content_tag("li", link_to(t("action.list"), config_milters_path)),
     content_tag("li", link_to(t("action.show"), config_milter_path)),
     content_tag("li", link_to(t("action.create"), new_config_milter_path))
    ]
  end

  def toggle_enabled_link(milter)
    if milter.enabled?
      link_to(t("action.disable"),
              {:id => milter, :action => "disable"},
              :method => :put)
    else
      link_to(t("action.enable"),
              {:id => milter, :action => "enable"},
              :method => :put)
    end
  end
end
