class Config::ApplicableConditionsController < ApplicationController
  before_filter :sync_configuration, :only => [:index]

  def index
    @conditions = Config::ApplicableCondition.find(:all)
  end

  def show
    @condition = Config::ApplicableCondition.find(params[:id])
  end
end
