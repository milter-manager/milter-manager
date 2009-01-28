class UsersController < ApplicationController
  skip_before_filter :login_required
  before_filter :login_required_when_no_users

  def new
    @user = User.new
  end

  def create
    @user = User.new(params[:user])
    success = @user && @user.save
    if success && @user.errors.empty?
      logout_keeping_session!
      # Protects against session fixation attacks, causes request forgery
      # protection if visitor resubmits an earlier form using back
      # button. Uncomment if you understand the tradeoffs.
      # reset session
      self.current_user = @user # !! now logged in
      redirect_back_or_default('/')
      flash[:notice] = t("notice.success.signup")
    else
      flash[:error]  = t("notice.failure.signup")
      render :action => 'new'
    end
  end

  private
  def login_required_when_no_users
    if User.count.zero?
      false
    else
      login_required
    end
  end
end
