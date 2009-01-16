// Place your application-specific JavaScript functions and classes here
// This file is automatically included by javascript_include_tag :defaults

function selectSpecInfoInputTag(type)
{
    switch (type) {
    case "inet":
    case "inet6":
        $("spec_ip_info_tag").show();
        $("spec_unix_info_tag").hide();
        break;
    case "unix":
        $("spec_ip_info_tag").hide();
        $("spec_unix_info_tag").show();
        break;
    }
}
