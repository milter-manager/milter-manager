# Methods added to this helper will be available to all templates in the application.
module ApplicationHelper
  def flash_box(message, options={})
    return '' if message.nil?
    id_suffix = options[:id_suffix] || ""
    id = "flash-box#{id_suffix}"
    fade_out = Proc.new {|page| page.visual_effect(:fade, id)}
    message = content_tag(:p, h(message), :class => "notice")
    flash_box_div = content_tag("div", "\n#{message}\n",
                                :id => id,
                                :class => "flash-box",
                                :onclick => update_page(&fade_out))
    if options[:need_container]
      flash_box_div = content_tag("div", "\n  #{flash_box_div}\n",
                                  :class => "flash-box-container")
    end
    set_opacity = update_page {|page| page[id].setOpacity("0.8")}
    effect = update_page do |page|
      page.delay(5) do
        fade_out.call(page)
      end
    end
    javascript_content = "#{set_opacity}\n#{effect}"
    "#{flash_box_div}\n#{javascript_tag(javascript_content)}"
  end

  def enabled_class(enabled)
    class_name = enabled ? "enabled" : "disabled"
    "class=\"#{class_name}\""
  end

  def enabled_label(enabled)
    if enabled
      t("label.enabled")
    else
      t("label.disabled")
    end
  end
end
