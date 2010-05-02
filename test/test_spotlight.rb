require "test/unit"
require "spotlight"

class TestSpotlight < Test::Unit::TestCase

  def setup
    @dir = File.expand_path(File.dirname(__FILE__))
    @file = File.join(@dir, 'test_file.txt')
  end

  def test_search
    assert_equal [@file], Spotlight.search("kMDItemDisplayName == 'test_file.txt'", @dir)
  end

  def test_attributes
    attributes = Spotlight.attributes(@file)
    assert_equal "test_file.txt", attributes['kMDItemDisplayName']
  end

  def test_get_attribute
    assert_equal "test_file.txt", Spotlight.get_attribute(@file, 'kMDItemDisplayName')
    assert_equal ["public.plain-text", "public.text", "public.data", "public.item", "public.content"], Spotlight.get_attribute(@file, 'kMDItemContentTypeTree')
  end

  def test_set_attribute
    assert_attribute_set('kMDItemTestStringType', "Time.now: #{Time.now}")
    assert_attribute_set('kMDItemTestLongType', Time.now.to_i)
    assert_attribute_set('kMDItemTestIntType', Time.now.to_i.to_s[-3..-1].to_i)
    assert_attribute_set('kMDItemTestFloatType', 1.123456688)
    assert_attribute_set('kMDItemTestArrayType', ["public.plain-text", "public.text"])
  end

  def assert_attribute_set(name, value)
    Spotlight.set_attribute(@file, name, value)
    # todo: will get latest time set value
    # need find out reason
    attr_value = Spotlight.get_attribute(@file, name)
    assert_equal value, attr_value
  end
end