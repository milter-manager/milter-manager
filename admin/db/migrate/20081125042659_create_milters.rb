class CreateMilters < ActiveRecord::Migration
  def self.up
    create_table :milters do |t|
      t.string :name
      t.string :command
      t.text :target_hosts
      t.text :target_addresses
      t.text :target_senders
      t.text :target_recipients
      t.text :target_headers

      t.timestamps
    end
  end

  def self.down
    drop_table :milters
  end
end
