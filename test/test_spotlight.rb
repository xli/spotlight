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

  def test_search_attribute_just_set
    time_now = Time.now.to_i
    Spotlight.set_attribute(@file, 'kMDItemNow', time_now)
    assert_equal [@file], Spotlight.search("kMDItemNow == #{time_now}", @dir)
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
    assert_attribute_set('kMDItemTestArrayTypeWithString', ["public.plain-text", "public.text"])
    assert_attribute_set('kMDItemTestArrayTypeWithLong', [123456789])
  end

  def test_set_date_attribute
    assert_attribute_set('kMDItemTestDateType', Time.now)
  end

  def assert_attribute_set(name, value)
    Spotlight.set_attribute(@file, name, value)
    attr_value = Spotlight.get_attribute(@file, name)
    assert_equal value, attr_value
  end
end