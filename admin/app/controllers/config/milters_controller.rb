class Config::MiltersController < ApplicationController
  before_filter :sync_configuration

  def index
    @milters = Config::Milter.find(:all, :order => :name)
  end

  def show
    @milter = Config::Milter.find(params[:id])
  end

  def new
    @milter = Config::Milter.new(params[:config_milter])
  end

  def edit
    @milter = Config::Milter.find(params[:id])
  end

  def create
    @milter = Config::Milter.new(params[:config_milter])

    if save_milter(@milter)
      flash[:notice] = t("notice.success.create", :target => t("target.milter"))
      redirect_to(@milter)
    else
      render :action => "new"
    end
  end

  def update
    @milter = Config::Milter.find(params[:id])

    config_milter = params[:config_milter] || {}
    applicable_conditions = params[:applicable_conditions] || []
    applicable_conditions = applicable_conditions.collect do |condition_id|
      Config::ApplicableCondition.find(Integer(condition_id))
    end
    config_milter[:applicable_conditions] = applicable_conditions

    @milter.attributes = config_milter
    if save_milter(@milter)
      flash[:notice] = t("notice.success.update", :target => t("target.milter"))
      redirect_to(@milter)
    else
      render :action => "edit"
    end
  end

  def destroy
    @milter = Config::Milter.find(params[:id])
    begin
      Config::Milter.transaction do
        @milter.destroy
        Configuration.update
      end
    rescue ::Milter::Manager::ConnectionError
      flash[:notice] = t("notice.failure.sync")
    end
    redirect_to(config_milters_url)
  end

  def enable
    milter = Config::Milter.find(params[:id])
    milter.enabled = true
    save_and_back_to_index(milter,
                           t("notice.success.enable",
                             :target => t("target.milter")))
  end

  def disable
    milter = Config::Milter.find(params[:id])
    milter.enabled = false
    save_and_back_to_index(milter,
                           t("notice.success.disable",
                             :target => t("target.milter")))
  end

  def enable_evaluation_mode
    milter = Config::Milter.find(params[:id])
    milter.evaluation_mode = true
    save_and_back_to_index(milter,
                           t("notice.success.enable-evaluation-mode",
                             :target => t("target.milter")))
  end

  def disable_evaluation_mode
    milter = Config::Milter.find(params[:id])
    milter.evaluation_mode = false
    save_and_back_to_index(milter,
                           t("notice.success.disable-evaluation-mode",
                             :target => t("target.milter")))
  end

  private
  def save_milter(milter)
    begin
      Config::Milter.transaction do
        milter.save!
        Configuration.update
        true
      end
    rescue ::Milter::Manager::Error, ActiveRecord::ActiveRecordError
      if $!.is_a?(::Milter::Manager::Error)
        milter.errors.add(:update,
                          "failed to update milter-manager's configuration: " +
                          $!.message)
      end
      false
    end
  end

  def save_and_back_to_index(milter, success_message)
    if save_milter(milter)
      flash[:notice] = success_message
    else
      flash[:error] = milter.errors.on(:update)
    end
    redirect_to(config_milters_path)
  end
end
