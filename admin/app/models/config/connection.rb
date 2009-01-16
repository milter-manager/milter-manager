class Config::Connection < ActiveRecord::Base
  DEFAULT_NAME = "default"

  validates_presence_of :spec
  validate :validate_spec
  after_save :clear_spec

  class << self
    def default
      connection = find_by_name(DEFAULT_NAME)
      if connection.nil?
        raise ActiveRecord::RecordNotFound, "default connection isn't found"
      end
      connection
    end
  end

  def control_client
    ::Milter::Manager::ControllerClient.new(spec)
  end

  def spec
    return nil if spec_type.blank?
    options = {
      :port => spec_port,
      :host => spec_host,
      :path => spec_path,
    }
    ::Milter::Manager::ConnectionSpec.build(spec_type, options).to_s
  end

  def spec=(spec)
    if spec.blank?
      type = nil
      options = {:port => nil, :host => nil, :path => nil}
    else
      begin
        spec = ::Milter::Manager::ConnectionSpec.parse(spec)
      rescue ArgumentError
        @spec = spec
        return
      end
      type, options = spec.decompose
    end

    self.spec_type = type
    self.spec_port = options[:port]
    self.spec_host = options[:host]
    self.spec_path = options[:path]
    @spec = nil
  end

  private
  def validate_spec
    @spec ||= nil
    _spec = @spec || spec
    return if _spec.blank?
    begin
      ::Milter::Manager::ConnectionSpec.parse(_spec)
    rescue ArgumentError
      errors.add("spec", $!.message)
    end
  end

  def clear_spec
    @spec = nil
  end
end
