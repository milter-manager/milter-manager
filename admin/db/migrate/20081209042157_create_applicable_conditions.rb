class CreateApplicableConditions < ActiveRecord::Migration
  def self.up
    create_table :applicable_conditions do |t|
      t.string :name
      t.text :description

      t.timestamps
    end
  end

  def self.down
    drop_table :applicable_conditions
  end
end
