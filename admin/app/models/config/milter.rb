class Config::Milter < ActiveRecord::Base
  FALLBACK_STATUSES = ["accept", "temporary_failure", "reject"]

  has_many :restrictions,
           :class_name => "Config::Restriction",
           :dependent => :destroy
  has_many :applicable_conditions, :through => :restrictions

  validates_presence_of :name, :connection_spec
  validate :validate_fallback_status
  validate :validate_connection_spec
  before_save :ensure_fallback_status
  after_save :clear_connection_spec

  class << self
    def apply(milters_config)
      return if milters_config.blank?

      milter_names = milters_config.collect {|m| m["name"]}
      destroy_all(["name NOT IN (?)", milter_names])
      milters_config.each do |milter|
        milter = milter.dup
        applicable_conditions = milter["applicable_conditions"] || []
        applicable_conditions = applicable_conditions.collect do |condition_name|
          condition = Config::ApplicableCondition.find_by_name(condition_name)
          if condition.nil?
            message = "Couldn't find #{self} with name=#{condition_name.inspect}"
            raise ActiveRecord::RecordNotFounc, message
          end
          condition
        end
        if applicable_conditions.empty?
          milter.delete("applicable_conditions")
        else
          milter["applicable_conditions"] = applicable_conditions
        end
        cached_milter = find_by_name(milter["name"])
        if cached_milter
          cached_milter.update_attributes!(milter)
        else
          create!(milter)
        end
      end
    end
  end

  def connection_spec
    return nil if connection_spec_type.blank?
    options = {
      :port => connection_spec_port,
      :host => connection_spec_host,
      :path => connection_spec_path,
    }
    ::Milter::Manager::ConnectionSpec.build(connection_spec_type, options).to_s
  end

  def connection_spec=(spec)
    @connection_spec = spec
    begin
      spec = ::Milter::Manager::ConnectionSpec.parse(spec)
    rescue ArgumentError
      return
    end
    type, options = spec.decompose
    self.connection_spec_type = type
    self.connection_spec_port = options[:port]
    self.connection_spec_host = options[:host]
    self.connection_spec_path = options[:path]
    @connection_spec = nil
  end

  def to_config_xml(indent=nil)
    Serializer.new(self, indent).to_xml
  end

  private
  def validate_connection_spec
    @connection_spec ||= nil
    spec = @connection_spec || connection_spec
    return if spec.blank?
    begin
      ::Milter::Manager::ConnectionSpec.parse(spec)
    rescue ArgumentError
      errors.add("connection_spec", $!.message)
    end
  end

  def validate_fallback_status
    return if fallback_status.nil?
    unless FALLBACK_STATUSES.include?(fallback_status)
      errors.add("fallback_status",
                 :not_available,
                 :available_list => FALLBACK_STATUSES.inspect)
    end
  end

  def ensure_fallback_status
    self.fallback_status ||= FALLBACK_STATUSES.first
  end

  def clear_connection_spec
    @connection_spec = nil
  end

  class Serializer
    include ERB::Util

    def initialize(milter, indent=nil)
      @milter = milter
      @indent = indent || 0
    end

    def to_xml
      tag("milter") do
        xml = content_tag("name", @milter.name)
        xml << content_tag("connection-spec", @milter.connection_spec)
        xml << content_tag("description", @milter.description)
        xml << content_boolean_tag("enabled", @milter.enabled)
        xml << content_timeout_tag("connection-timeout",
                                   @milter.connection_timeout)
        xml << content_timeout_tag("writing-timeout",
                                   @milter.writing_timeout)
        xml << content_timeout_tag("reading-timeout",
                                   @milter.reading_timeout)
        xml << content_timeout_tag("end-of-message-timeout",
                                   @milter.end_of_message_timeout)
        xml << content_tag("user-name", @milter.user_name)
        xml << content_tag("command", @milter.command)
        xml << content_tag("command_options", @milter.command_options)
        unless @milter.applicable_conditions.empty?
          xml << tag("applicable-conditions") do
            @milter.applicable_conditions.collect do |condition|
              content_tag("applicable-condition", condition.name)
            end.join("")
          end
        end
        xml
      end
    end

    private
    def tag(name)
      xml = ""
      xml << "#{indent}<#{name}>\n"
      indent do
        xml << yield
      end
      xml << "#{indent}</#{name}>\n"
      xml
    end

    def content_tag(name, content)
      if content.blank?
        ""
      else
        "#{indent}<#{name}>#{h(content)}</#{name}>\n"
      end
    end

    def content_boolean_tag(name, boolean)
      if boolean.nil?
        content = nil
      else
        content = boolean ? "true" : "false"
      end
      content_tag(name, content)
    end

    def content_timeout_tag(name, timeout)
      if timeout.nil? or timeout <= 0.0
        ""
      else
        content_tag(name, timeout.to_s)
      end
    end

    def indent(level=0)
      if block_given?
        indent = @indent
        @indent += 2
        begin
          yield
        ensure
          @indent = indent
        end
      else
        " " * @indent
      end
    end
  end
end
