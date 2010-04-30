require "test/unit"
require "spotlight"

class TestSpotlight < Test::Unit::TestCase
  def test_search
    assert_equal [__FILE__], Spotlight.search("kMDItemDisplayName == 'spotlight_test.rb'", File.dirname(__FILE__))
  end
end