require 'test_helper'

class Config::ConnectionTest < ActiveSupport::TestCase
  def test_validation
    assert_valid_model(new_connection(:spec => "unix:/tmp/socket"))
    assert_valid_model(new_connection(:spec_type => "inet",
                                      :spec_port => 2929))
    assert_valid_model(new_connection(:spec_type => "inet",
                                      :spec_port => 2929,
                                      :spec_host => "example.com"))
    assert_invalid_model({
                           "spec" => [t_ar("errors.messages.blank")],
                         },
                         new_connection)
    assert_invalid_model({
                           "spec" => [t_ar("errors.messages.blank"),
                                      "invalid spec type: " +
                                      "<unknown>: " +
                                      "<unknown:/tmp/socket>"],
                         },
                         new_connection(:spec => "unknown:/tmp/socket"))
  end

  private
  def new_connection(attributes={})
    Config::Connection.new(attributes)
  end
end
