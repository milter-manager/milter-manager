class CreateConfigurations < ActiveRecord::Migration
  def self.up
    create_table :configurations do |t|
      t.text :content
      t.boolean :applied
      t.datetime :modified_at

      t.timestamps
    end
  end

  def self.down
    drop_table :configurations
  end
end
