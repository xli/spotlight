require "test/unit"
require "spotlight"

class TestSpotlight < Test::Unit::TestCase
  include Spotlight
  def test_search
    assert_equal [__FILE__], search("kMDItemDisplayName == 'spotlight_test.rb'", File.dirname(__FILE__))
  end
end