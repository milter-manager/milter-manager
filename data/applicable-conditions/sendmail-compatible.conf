# -*- ruby -*-

define_applicable_condition("Sendmail Compatible") do |condition|
  condition.description =
    "Make a milter depends on Sendmail workable with Postfix"

  condition.define_connect_stopper do |context, host, address|
    if context.postfix?
      context["if_addr"] ||= context["client_addr"]
      context["if_name"] ||= context["client_name"]
    end
    false
  end

  condition.define_envelope_from_stopper do |context, from|
    context["i"] ||= "dummy-id" if context.postfix?
    false
  end
end
