require 'spotlight/spotlight'

module Spotlight
  VERSION = "0.0.3"

  def self.search(query_string, *scope_directories)
    Intern.search(query_string, scope_directories).collect {|path| MDItem.new(path)}
  end

  class MDItem
    VALID_ATTRIBUTE_VALUE_TYPE = [String, Time, Fixnum, Bignum, Float, Array, NilClass, TrueClass, FalseClass]

    class InvalidAttributeValueTypeError < StandardError
    end

    attr_reader :path
    def initialize(path)
      raise ArgumentError, 'Path is nil' if path.nil?
      @path = path
    end
    def attributes
      Intern.attributes(@path)
    end
    def attributes=(attributes)
      Intern.set_attributes(@path, attributes)
    end
    def [](attr_name)
      Intern.get_attribute(@path, attr_name)
    end
    def []=(attr_name, attr_value)
      validate(attr_value)
      Intern.set_attribute(@path, attr_name, attr_value)
    end
    def reload
      self
    end
    def ==(obj)
      obj.is_a?(MDItem) && @path == obj.path
    end

    def validate(value)
      raise InvalidAttributeValueTypeError, "Invalid attribute value type #{value.class.inspect}, valid types: #{VALID_ATTRIBUTE_VALUE_TYPE.join(", ")}" unless VALID_ATTRIBUTE_VALUE_TYPE.include?(value.class)
    end
  end
end
