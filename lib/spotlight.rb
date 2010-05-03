require 'spotlight/spotlight'

module Spotlight
  def self.search(query_string, scope_directory)
    Intern.search(query_string, scope_directory).collect {|path| MDItem.new(path)}
  end

  class MDItem
    attr_reader :path
    def initialize(path)
      @path = path
    end
    def attributes
      Intern.attributes(@path)
    end
    def [](attr_name)
      Intern.get_attribute(@path, attr_name)
    end
    def []=(attr_name, attr_value)
      Intern.set_attribute(@path, attr_name, attr_value)
    end
    def reload
      self
    end
    def ==(obj)
      obj.is_a?(MDItem) && @path == obj.path
    end
  end
end
