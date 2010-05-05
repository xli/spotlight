
ENV["ARCHFLAGS"]="-arch i386"

require 'rubygems'
require 'echoe'
require 'rake/extensiontask'

Echoe.new('mac-spotlight', '0.0.2') do |p|
  p.description     = "Spotlight - Ruby interface to Mac OSX Spotlight"
  p.url             = "https://github.com/xli/spotlight"
  p.author          = "Li Xiao"
  p.email           = "iam@li-xiao.com"
  p.ignore_pattern  = "*.gemspec"
  p.development_dependencies = ['rake', 'echoe', 'rake-compiler']
  p.test_pattern = "test/test_*.rb"
  p.rdoc_options    = %w(--main README.rdoc --inline-source --line-numbers --charset UTF-8)
end

Rake::ExtensionTask.new('spotlight') do |ext|
  ext.lib_dir = 'lib/spotlight'
end
