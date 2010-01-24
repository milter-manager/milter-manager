# Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

module Milter::Manager
  class PolicyManager
    def initialize(loader)
      @loader = loader
      @policies = []
    end

    def <<(policy)
      @policies << policy
    end

    def apply
      return if @policies.empty?

      condition_name = "Policy"
      @loader.define_applicable_condition(condition_name) do |condition|
        condition.description = "Apply policies defined by define_policy"

        condition.define_connect_stopper do |context, host, address|
          stop_on_connect?(context, host, address)
        end

        condition.define_helo_stopper do |context, fqdn|
          stop_on_helo?(context, fqdn)
        end

        condition.define_envelope_from_stopper do |context, from|
          stop_on_envelope_from?(context, from)
        end

        condition.define_envelope_recipient_stopper do |context, recipient|
          stop_on_envelope_recipient?(context, recipient)
        end

        condition.define_data_stopper do |context|
          stop_on_data?(context)
        end
      end

      @loader.defined_milters.each do |name|
        @loader.define_milter(name) do |milter|
          milter.add_applicable_condition(condition_name)
        end
      end
    end

    def stop_on_connect?(context, host, address)
      @policies.any? do |policy|
        policy.stop_on_connect?(context, host, address)
      end
    end

    def stop_on_helo?(context, fqdn)
      @policies.any? do |policy|
        policy.stop_on_helo?(context, helo)
      end
    end

    def stop_on_envelope_from?(context, from)
      @policies.any? do |policy|
        policy.stop_on_envelope_from?(context, from)
      end
    end

    def stop_on_envelope_recipient?(context, recipient)
      @policies.any? do |policy|
        policy.stop_on_envelope_recipient?(context, recipient)
      end
    end

    def stop_on_data?(context)
      @policies.any? do |policy|
        policy.stop_on_data?(context)
      end
    end
  end

  class Policy
    attr_accessor :milters, :host, :address, :fqdn, :from, :recipient
    def initialize(name)
      @name = name
      @milters = []
      @host = nil
      @address = nil
      @fqdn = nil
      @from = nil
      @recipient = nil
    end

    def stop_on_connect?(context, host, address)
      return false unless target_milter?(context)
      not (@host === host or @address === address)
    end

    def stop_on_helo?(context, fqdn)
      return false unless target_milter?(context)
      not (@fqdn === fqdn)
    end

    def stop_on_envelope_from?(context, from)
      return false unless target_milter?(context)
      not (@from === from)
    end

    def stop_on_envelope_recipient?(context, recipient)
      return false unless target_milter?(context)
      not (@recipient === recipient)
    end

    def stop_on_data?(context)
      not target_milter?(context)
    end

    private
    def target_milter?(context)
      @milters.include?(context.name)
    end
  end
end
