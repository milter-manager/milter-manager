# Filters added to this controller apply to all controllers in the application.
# Likewise, all the methods added will be available for all controllers.

class ApplicationController < ActionController::Base
  include AuthenticatedSystem
  begin
    include ExceptionNotifiable
  rescue NameError
    # for rake gems:install :<
  end

  helper :all # include all helpers, all the time

  # See ActionController::RequestForgeryProtection for details
  # Uncomment the :secret if you're not using the cookie session store
  protect_from_forgery # :secret => '3b815e5865917c8c6e7a566a94ecbdef'
  
  # See ActionController::Base for details 
  # Uncomment this to filter the contents of submitted sensitive data parameters
  # from your application log (in this case, all fields with names like "password"). 
  filter_parameter_logging :password, :password_confirmation

  before_filter :login_required

  private
  def sync_configuration
    configuration = nil
    begin
      Configuration.sync
      configuration = Configuration.latest
    rescue ::Milter::Manager::ConnectionError
      flash[:notice] = t("notice.failure.milter-manager-connect",
                         :message => $!.message)
      logger.info($!)
    end

    if configuration and !configuration.applied?
      configuration.apply
    end

    true
  end

  def render_404(exception)
    init_locale
    render(:file => "/rescues/404",
           :layout => "application",
           :status => "404 Not Found",
           :locals => {:exception => exception})
  end

  def render_500(exception)
    init_locale
    render(:file => "/rescues/500",
           :layout => "application",
           :status => "500 Error",
           :locals => {:exception => exception})
  end

  def rescue_action_in_public(exception)
    case exception
    when *self.class.exceptions_to_treat_as_404
      render_404(exception)
    else
      render_500(exception)

      deliverer = self.class.exception_data
      data = case deliverer
             when nil then {}
             when Symbol then send(deliverer)
             when Proc then deliverer.call(self)
             end

      ExceptionNotifier.deliver_exception_notification(exception, self, request, data)
    end
  end
end
