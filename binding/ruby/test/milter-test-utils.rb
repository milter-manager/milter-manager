require 'socket'
require 'tempfile'

module MilterTestUtils
  include RR::Adapters::RRMethods
  RR.trim_backtrace = true

  def self.included(mod)
    return unless mod < Test::Unit::TestCase
    mod.class_eval do
      setup
      def setup_rr
        RR.reset
      end

      teardown
      def teardown_rr
        RR.verify
      end
    end
  end
end

require 'milter-encoder-test-utils'
require 'milter-manager-encoder-test-utils'
