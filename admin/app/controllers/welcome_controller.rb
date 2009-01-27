class WelcomeController < ApplicationController
  def index
    connection = Config::Connection.default
    if connection.spec.nil?
      redirect_to(edit_config_connection_path(connection))
    end
  end
end
