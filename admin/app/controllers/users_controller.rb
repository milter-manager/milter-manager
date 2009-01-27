class UsersController < ApplicationController
  skip_before_filter :login_required

  # render new.rhtml
  def new
    raise "XXX"
    @user = User.new
  end
 
  def create
    logout_keeping_session!
    @user = User.new(params[:user])
    success = @user && @user.save
    if success && @user.errors.empty?
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
end
