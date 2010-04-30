
require 'rubygems'
require 'echoe'
require 'rake/extensiontask'

Echoe.new('spotlight', '1.0.0') do |p|
  p.description     = "Spotlight is a ruby gem wrapped Mac OS X Spotlight"
  p.url             = "https://github.com/xli/spotlight"
  p.author          = "Li Xiao"
  p.email           = "iam@li-xiao.com"
  p.ignore_pattern  = "*.gemspec"
  p.runtime_dependencies = ["activesupport"]
  p.development_dependencies = ['rake', 'echoe']
  p.test_pattern = "test/*_test.rb"
  p.platform = "darwin"
  p.rdoc_options    = %w(--main README.rdoc --inline-source --line-numbers --charset UTF-8)
end

Rake::ExtensionTask.new('spotlight')