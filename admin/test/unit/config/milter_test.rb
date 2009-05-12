require 'test_helper'

class Config::MilterTest < ActiveSupport::TestCase
  def test_validation
    assert_valid_model(new_milter(:name => "milter",
                                  :connection_spec => "unix:/tmp/socket"))
    assert_invalid_model({
                           "name" => [t_ar("errors.messages.blank")],
                           "connection_spec" => [t_ar("errors.messages.blank")],
                         },
                         new_milter)
    assert_invalid_model({
                           "connection_spec" => [t_ar("errors.messages.blank"),
                                                 "invalid spec type: " +
                                                 "<unknown>: " +
                                                 "<unknown:/tmp/socket>"],
                         },
                         new_milter(:name => "milter",
                                    :connection_spec => "unknown:/tmp/socket"))
    assert_invalid_model({
                           "fallback_status" =>
                           [t_ar("errors.models.config/milter.attributes.fallback_status.not_available",
                              :available_list => ["accept",
                                                  "temporary-failure",
                                                  "reject"].inspect,
                              :value => "unknown")],
                         },
                         new_milter(:name => "milter",
                                    :connection_spec => "unix:/tmp/socket",
                                    :fallback_status => "unknown"))
  end

  def test_apply
    anti_spam = milters(:anti_spam)
    greylist = milters(:greylist)
    virus_scanner = milters(:virus_scanner)
    assert_equal_milters([anti_spam, greylist, virus_scanner],
                         Config::Milter.find(:all))

    new_milter_config = {
      "name" => "new milter",
      "connection_spec_type" => "unix",
      "connection_spec_path" => "/tmp/milter.sock",
      "applicable_conditions" => ["S25R", "remote-network"],
    }
    change_milter_config = {
      "name" => anti_spam.name,
      "connection_spec" => "inet:10025@unknown.example.org",
    }
    same_milter_config = {
      "name" => virus_scanner.name,
      "connection_spec" => virus_scanner.connection_spec,
    }
    Config::Milter.apply([new_milter_config,
                          change_milter_config,
                          same_milter_config])

    change_milter_config_for_new =
      change_milter_config.merge(:applicable_conditions =>
                                 [applicable_conditions(:remote_network)])
    change_milter_config_for_new =
      anti_spam.attributes.merge(change_milter_config_for_new)
    changed_anti_spam = Config::Milter.new(change_milter_config_for_new)
    new_milter_config_for_new =
      new_milter_config.merge(:applicable_conditions =>
                              [applicable_conditions(:s25r),
                               applicable_conditions(:remote_network)])
    new_milter = Config::Milter.new(new_milter_config_for_new)
    assert_equal_milters([changed_anti_spam, virus_scanner, new_milter],
                         Config::Milter.find(:all))
  end

  def test_relation
    virus_scanner = milters(:virus_scanner)

    s25r = applicable_conditions(:s25r)
    assert_equal([s25r], virus_scanner.applicable_conditions)

    remote_network = applicable_conditions(:remote_network)
    virus_scanner.applicable_conditions << remote_network
    virus_scanner = Config::Milter.find(virus_scanner)
    assert_equal([s25r, remote_network], virus_scanner.applicable_conditions)

    assert_difference("Config::Restriction.count", -2) do
      virus_scanner.destroy
    end
  end

  def test_to_config_xml
    assert_equal(<<-EOX, milters(:virus_scanner).to_config_xml(4))
    <milter>
      <name>virus scanner</name>
      <connection-spec>inet:10028@localhost</connection-spec>
      <command>virus-scan-milter</command>
      <applicable-conditions>
        <applicable-condition>S25R</applicable-condition>
      </applicable-conditions>
    </milter>
EOX
  end

  def test_to_config_xml_full
    milter = milters(:virus_scanner)
    milter.description = "virus scan milter"
    milter.enabled = true
    milter.connection_timeout = 300
    milter.writing_timeout = 30
    milter.reading_timeout = 20
    milter.end_of_message_timeout = 900
    milter.user_name = "virus-scan"
    assert_equal(<<-EOX, milter.to_config_xml(4))
    <milter>
      <name>virus scanner</name>
      <connection-spec>inet:10028@localhost</connection-spec>
      <description>virus scan milter</description>
      <enabled>true</enabled>
      <connection-timeout>300.0</connection-timeout>
      <writing-timeout>30.0</writing-timeout>
      <reading-timeout>20.0</reading-timeout>
      <end-of-message-timeout>900.0</end-of-message-timeout>
      <user-name>virus-scan</user-name>
      <command>virus-scan-milter</command>
      <applicable-conditions>
        <applicable-condition>S25R</applicable-condition>
      </applicable-conditions>
    </milter>
EOX
  end

  private
  def assert_equal_milters(expected, actual)
    assert_equal(normalize_milters(expected),
                 normalize_milters(actual))
  end

  def normalize_milters(milters)
    milters.collect do |milter|
      {
        :name => milter.name,
        :command => milter.command,
        :connection_spec => milter.connection_spec,
        :applicable_conditions => milter.applicable_conditions.collect(&:name),
      }
    end.sort_by do |milter|
      milter[:name]
    end
  end

  def new_milter(attributes={})
    Config::Milter.new(attributes)
  end
end
