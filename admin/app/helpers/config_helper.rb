module ConfigHelper
  def config_menus(indent='')
    [content_tag(:h3, t("title.config-items")),
     content_tag(:ul) do
       items = [link_to(t("menu.config-connection"), Config::Connection.default)]
       unless Config::Connection.default.spec.blank?
         items.concat([link_to(t("menu.config-milter"), config_milters_path),
                       link_to(t("menu.config-applicable-condition"),
                               config_applicable_conditions_path)])
       end
       items = items.collect do |link|
         "#{indent}  <li>#{link}</li>"
       end.join("\n")
       "\n#{items}\n#{indent}"
     end].collect do |line|
      "#{indent}#{line}"
    end.join("\n")
  end

  def config_sub_menus(indent='')
    sub_menu_generator = [controller.controller_path.gsub(/\//, "_"),
                          action_name,
                          "menus"].join("_")
    if respond_to?(sub_menu_generator)
      [content_tag(:h3, t("title.operations")),
       content_tag(:ul) do
         items = send(sub_menu_generator).collect do |item|
           "#{indent}  #{item}"
         end.join("\n")
         "\n#{items}\n#{indent}"
       end].collect do |line|
        "#{indent}#{line}"
      end.join("\n")
    else
      ''
    end
  end

  def spec_input_tag(form, prefix=nil)
    specs = [
             ["unix", "unix"],
             ["inet", "inet"],
             ["inet6", "inet6"],
            ]
    result = form.label("#{prefix}spec_type", t("label.type")) + ":\n"
    result << form.select("#{prefix}spec_type",
                          specs,
                          {},
                          :onchange => "selectSpecInfoInputTag(this.value)")
    result << "\n&nbsp;\n"
    result << content_tag("span",
                          form.label("#{prefix}spec_port", t("label.port")) +
                          ":" +
                          form.text_field("#{prefix}spec_port", :size => 5) +
                          "\n&nbsp;\n" +
                          form.label("#{prefix}spec_host", t("label.host")) +
                          ":" +
                          form.text_field("#{prefix}spec_host") +
                          "\n",
                          :id => "spec_ip_info_tag")
    result << "\n"
    result << content_tag("span",
                          form.label("#{prefix}spec_path", t("label.path")) +
                          ":" +
                          form.text_field("#{prefix}spec_path"),
                          :id => "spec_unix_info_tag")
    result << "\n"
    spec_type = form.object.send("#{prefix}spec_type") || specs[0][0]
    result << javascript_tag("selectSpecInfoInputTag(#{spec_type.dump})")
    result << "\n"
    result
  end
end
