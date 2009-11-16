class Configuration < ActiveRecord::Base
  class << self
    def latest
      find(:first, :order => "modified_at DESC, updated_at DESC") ||
        create(:modified_at => Time.at(0))
    end

    def sync
      client = Config::Connection.default.control_client
      manager_config = client.configuration
      last_modified = manager_config["last_modified"] || Time.now
      config = find_by_modified_at(last_modified)
      return if config and config.content == manager_config

      config ||= create(:modified_at => last_modified)
      config.content = manager_config
      config.applied = false
      config.save!
    end

    def update
      return unless modified?

      config_xml = "<configuration>\n"
      config_xml << "  <milters>\n"
      Config::Milter.find(:all).each do |milter|
        config_xml << milter.to_config_xml(4)
      end
      config_xml << "  </milters>\n"
      config_xml << "</configuration>\n"
      client = Config::Connection.default.control_client
      client.configuration = config_xml
      client.reload
    end

    def modified?
      config = latest
      return true if config.modified_at.nil?

      config_milters = (config.content || {})["milters"] || []
      Config::Milter.count != config_milters.size or
        Config::Milter.exists?(["updated_at > ?", config.modified_at])
    end
  end

  serialize :content

  def apply
    return if content.blank?
    ActiveRecord::Base.transaction do
      Config::ApplicableCondition.apply(content["applicable_conditions"])
      Config::Milter.apply(content["milters"])
      self.update_attribute(:applied, true)
    end
  end
end
