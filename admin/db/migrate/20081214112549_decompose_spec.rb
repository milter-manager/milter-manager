class DecomposeSpec < ActiveRecord::Migration
  def self.up
    [
     ["milters", Config::Milter, "connection_"],
     ["connections", Config::Connection, ""]
    ].each do |table_name, model, prefix|
      add_column table_name, "#{prefix}spec_type", :text
      add_column table_name, "#{prefix}spec_port", :integer
      add_column table_name, "#{prefix}spec_host", :text
      add_column table_name, "#{prefix}spec_path", :text
      model.find(:all).each do |record|
        spec_text = record.send("#{prefix}spec")
        next if spec_text.blank?
        spec = ::Milter::Manager::ConnectionSpec.parse(spec_text)
        type, options = spec.decompose
        record.send("#{prefix}spec_type=", type)
        record.send("#{prefix}spec_port=", options[:port])
        record.send("#{prefix}spec_host=", options[:host])
        record.send("#{prefix}spec_path=", options[:path])
        record.save!
      end
      remove_column table_name, "#{prefix}spec"
    end
  end

  def self.down
    [
     ["milters", Config::Milter, "connection_"],
     ["connections", Config::Connection, ""]
    ].each do |table_name, model, prefix|
      add_column table_name, "#{prefix}spec", :text
      model.find(:all).each do |record|
        spec_type = record.send("#{prefix}spec_type")
        next if spec_type.blank?
        spec_options = {
          :port => record.send("#{prefix}spec_port"),
          :host => record.send("#{prefix}spec_host"),
          :path => record.send("#{prefix}spec_path"),
        }
        spec = ::Milter::Manager::ConnectionSpec.build(spec_type,
                                                       spec_options)
        record.send("#{prefix}spec=", spec.to_s)
        record.save!
      end
      remove_column table_name, "#{prefix}spec_type"
      remove_column table_name, "#{prefix}spec_port"
      remove_column table_name, "#{prefix}spec_host"
      remove_column table_name, "#{prefix}spec_path"
    end
  end
end
