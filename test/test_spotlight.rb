require "test/unit"
require "spotlight"

class TestSpotlight < Test::Unit::TestCase

  def setup
    @dir = File.expand_path(File.dirname(__FILE__))
    @item = Spotlight::MDItem.new(File.join(@dir, 'test_file.txt'))
  end

  def test_search
    assert_equal [@item], Spotlight.search("kMDItemDisplayName == 'test_file.txt'", @dir)
  end

  def test_search_with_multi_scope_directories
    @lib_dir = File.expand_path(File.dirname(__FILE__) + "/../lib")
    assert_equal [@item], Spotlight.search("kMDItemDisplayName == 'test_file.txt'", @lib_dir, @dir)
  end

  def test_search_attribute_just_set
    time_now = Time.now.to_i
    @item['kMDItemNow'] = time_now
    assert_equal [@item], Spotlight.search("kMDItemNow == #{time_now}", @dir)
  end

  def test_search_with_multi_attribute
    @item['kMDItemAttr1'] = 1
    @item['kMDItemAttr2'] = 2
    assert_equal [], Spotlight.search("kMDItemAttr1 == 2 && kMDItemAttr2 == 3", @dir)
    assert_equal [], Spotlight.search("kMDItemAttr1 == 1 && kMDItemAttr2 == 3", @dir)
    assert_equal [@item], Spotlight.search("kMDItemAttr1 == 1 && kMDItemAttr2 == 2", @dir)
  end

  def test_attributes
    assert_equal "test_file.txt", @item.attributes['kMDItemDisplayName']
  end

  def test_could_not_set_attribute_value_when_it_is_not_time_string_int_and_float
    assert_raise Spotlight::MDItem::InvalidAttributeValueTypeError do
      assert_attribute_set('kMDItemTestLongType', Hash.new)
    end
  end

  def test_get_attribute
    assert_equal "test_file.txt", @item['kMDItemDisplayName']
    assert_equal ["public.plain-text", "public.text", "public.data", "public.item", "public.content"], @item['kMDItemContentTypeTree']
  end

  def test_set_attribute
    assert_attribute_set('kMDItemTestNilType', nil)
    assert_attribute_set('kMDItemTestStringType', "Time.now: #{Time.now}")
    assert_attribute_set('kMDItemTestLongType', Time.now.to_i)
    assert_attribute_set('kMDItemTestIntType', Time.now.to_i.to_s[-3..-1].to_i)
    assert_attribute_set('kMDItemTestFloatType', 1.123456688)
    assert_attribute_set('kMDItemTestArrayTypeWithString', ["public.plain-text", "public.text"])
    assert_attribute_set('kMDItemTestArrayTypeWithLong', [123456789])
  end

  def test_set_attributes
    attributes = {'TimeNow' => "Time.now: #{Time.now}", 'SomeThing' => Time.now.to_i}
    @item.attributes = attributes
    attributes.each do |key, value|
      assert_equal value, @item.reload[key]
    end
  end

  def test_set_date_attribute
    assert_attribute_set('kMDItemTestDateType', Time.now)
  end

  def assert_attribute_set(name, value)
    @item[name] = value
    assert_equal value, @item.reload[name]
  end
end