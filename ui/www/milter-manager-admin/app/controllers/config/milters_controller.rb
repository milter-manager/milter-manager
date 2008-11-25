class Config::MiltersController < ApplicationController
  def index
    @milters = Milter.find(:all)
  end

  def show
    @milter = Milter.find(params[:id])
  end

  def new
    @milter = Milter.new(params[:milter])
  end

  def edit
    @milter = Milter.find(params[:id])
  end

  def create
    @milter = Milter.new(params[:milter])

    if @milter.save
      flash[:notice] = "User was successfully created."
      redirect_to(config_milter_path(@milter))
    else
      render :action => "new"
    end
  end

  def update
    @milter = Milter.find(params[:id])

    if @milter.update_attributes(params[:milter])
      flash[:notice] = "User was successfully updated."
      redirect_to(config_milter_path(@milter))
    else
      render :action => "edit"
    end
  end
end
