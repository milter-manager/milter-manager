require 'test_helper'

class Config::RestrictionTest < ActiveSupport::TestCase
  def test_relation
    virus_scan_s25r = restrictions(:virus_scan_s25r)

    assert_equal(milters(:virus_scanner), virus_scan_s25r.milter)
    assert_equal(applicable_conditions(:s25r),
                 virus_scan_s25r.applicable_condition)
  end
end
