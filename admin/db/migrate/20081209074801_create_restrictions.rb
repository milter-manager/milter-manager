class CreateRestrictions < ActiveRecord::Migration
  def self.up
    create_table :restrictions do |t|
      t.integer :milter_id
      t.integer :applicable_condition_id

      t.timestamps
    end
  end

  def self.down
    drop_table :config_restrictions
  end
end
