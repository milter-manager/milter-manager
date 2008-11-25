# -*- ruby -*-

namespace :test do
  Rake::TestTask.new(:all => "db:test:prepare") do |t|
    t.libs << 'test'
    t.verbose = false
    t.test_files = 'test/**/*_test.rb'
  end
  Rake::Task['test:all'].comment = "Test all"
end
