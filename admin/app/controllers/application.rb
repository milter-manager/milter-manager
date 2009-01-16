# Filters added to this controller apply to all controllers in the application.
# Likewise, all the methods added will be available for all controllers.

class ApplicationController < ActionController::Base
  helper :all # include all helpers, all the time

  # See ActionController::RequestForgeryProtection for details
  # Uncomment the :secret if you're not using the cookie session store
  protect_from_forgery # :secret => '3b815e5865917c8c6e7a566a94ecbdef'
  
  # See ActionController::Base for details 
  # Uncomment this to filter the contents of submitted sensitive data parameters
  # from your application log (in this case, all fields with names like "password"). 
  # filter_parameter_logging :password

  private
  def sync_configuration
    configuration = nil
    begin
      Configuration.sync
      configuration = Configuration.latest
    rescue ::Milter::Manager::ConnectionError
      logger.info($!)
    end

    if configuration and !configuration.applied?
      configuration.apply
    end

    true
  end
end
