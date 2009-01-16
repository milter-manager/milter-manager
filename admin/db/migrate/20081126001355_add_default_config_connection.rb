class AddDefaultConfigConnection < ActiveRecord::Migration
  def self.up
    begin
      Config::Connection.default
    rescue ActiveRecord::RecordNotFound
      connection = Config::Connection.new
      connection[:name] = Config::Connection::DEFAULT_NAME
      connection[:spec] = "unix:/tmp/milter-manager-admin.sock"
      connection.save(false)
    end
  end

  def self.down
    Config::Connection.default.destroy
  rescue ActiveRecord::RecordNotFound
  end
end
