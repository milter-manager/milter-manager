class Config::ConnectionsController < ApplicationController
  def show
    @connection = Config::Connection.find(params[:id])
  end

  def edit
    @connection = Config::Connection.find(params[:id])
  end

  def update
    @connection = Config::Connection.find(params[:id])
    if @connection.update_attributes(params[:config_connection])
      flash[:notice] = t("notice.success.update",
                         :target => t("title.connection"))
      redirect_to(@connection)
    else
      render :action => "edit"
    end
  end
end
