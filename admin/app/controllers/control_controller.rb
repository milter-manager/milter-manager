class ControlController < ApplicationController
  def index
    client = Config::Connection.default.control_client
    begin
      @status = client.status
    rescue Milter::Manager::ConnectionError
      @status = "unconnected: <#{$!}>"
    end
  end
end
