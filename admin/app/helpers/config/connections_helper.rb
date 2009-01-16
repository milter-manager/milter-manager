module Config::ConnectionsHelper
  def config_connections_show_menus
    [content_tag("li", link_to(t("action.edit"), edit_config_connection_path))]
  end

  def config_connections_edit_menus
    [content_tag("li", link_to(t("action.show"), config_connection_path))]
  end
end
