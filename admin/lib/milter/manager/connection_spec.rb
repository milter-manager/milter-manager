require 'socket'

module Milter
  module Manager
    class ConnectionSpec
      class << self
        def parse(spec)
          type, content = spec.split(/:/, 2)
          if content.nil?
            raise ArgumentError, "separator colon is missing: <#{spec}>"
          end
          case type
          when "unix", "local"
            if content.blank?
              raise ArgumentError,
                    "UNIX domain socket path is missing: <#{spec}>"
            end
            type = "unix"
            options = {:path => content}
          when "inet", "inet6"
            if content.blank?
              raise ArgumentError, "port number is missing: <#{spec}>"
            end
            port, host = content.split(/@/, 2)
            options = {:port => Integer(port), :host => host}
          else
            raise ArgumentError, "invalid spec type: <#{type}>: <#{spec}>"
          end
          build(type, options)
        end

        def build(type, options)
          case type.to_s
          when "unix"
            UNIXConnectionSpec.new(options[:path])
          when "inet"
            InetConnectionSpec.new(options[:port], options[:host])
          when "inet6"
            Inet6ConnectionSpec.new(options[:port], options[:host])
          else
            raise ArgumentError,
                  "invalid spec type: <#{type.inspect}>: <#{options.inspect}>"
          end
        end
      end
    end

    class UNIXConnectionSpec
      attr_accessor :path
      def initialize(path)
        if path.blank?
          raise ArgumentError, "path should not be blank: <#{path.inspect}>"
        end
        @path = path
      end

      def open(&block)
        UNIXSocket.open(@path, &block)
      end

      def decompose
        ["unix", {:path => @path}]
      end

      def to_s
        "unix:#{@path}"
      end

      def ==(other)
        super(other) or
          (self.class === other and @path == other.path)
      end
    end

    class IPConnectionSpec
      attr_reader :type
      protected :type

      attr_accessor :port, :host
      def initialize(type, port, host=nil)
        if port.blank?
          raise ArgumentError, "port should not be blank: <#{port.inspect}>"
        end
        @type = type
        @port = port
        @host = host
      end

      def open(&block)
        TCPSocket.open(@host || default_host, @port, &block)
      end

      def decompose
        options = {:port => port}
        options[:host] = @host if @host
        [@type, options]
      end

      def to_s
        string = "#{@type}:#{@port}"
        string << "@#{@host}" unless @host.blank?
        string
      end

      def ==(other)
        super(other) or
          (self.class === other and
           [@type, @port, @host] ==
           [other.type, other.port, other.host])
      end
    end

    class InetConnectionSpec < IPConnectionSpec
      def initialize(port, host=nil)
        super("inet", port, host)
      end

      private
      def default_host
        "127.0.0.1"
      end
    end

    class Inet6ConnectionSpec < IPConnectionSpec
      def initialize(port, host=nil)
        super("inet6", port, host)
      end

      private
      def default_host
        "::1"
      end
    end
  end
end
