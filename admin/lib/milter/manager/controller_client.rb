module Milter
  module Manager
    class ControllerClient
      class Error < Manager::Error
      end

      def initialize(spec)
        @spec = ConnectionSpec.parse(spec)
        @server = nil
      end

      def status
        send_packet(packet("get-status"))
      end

      def configuration
        send_packet(packet("get-configuration"))
      end

      def configuration=(config)
        send_packet(packet("set-configuration", config))
      end

      def reload
        send_packet(packet("reload"))
      end

      private
      def send_packet(_packet)
        retried = false
        begin
          @server ||= @spec.open
          @server.print(_packet)
          @server.flush
          decode(@server)
        rescue IOError
          raise if retried
          retried = true
          @server = nil
          retry
        end
      rescue SystemCallError
        backtrace = ["#{__FILE__}:#{__LINE__}:in `send_packet'"] + $@
        raise ConnectionError, $!.message, backtrace
      end

      def packet(command, content='')
        pack(command + "\0" + content)
      end

      def pack(content)
        [content.size].pack("N") + content
      end

      def decode(server)
        size = nil
        buffer = nil
        command = nil
        content = nil
        while data = read_data(server)
          if size.nil?
            size = data.unpack("N")[0]
            data = data[4, data.size - 4]
          end
          if buffer.nil?
            buffer = data
          else
            buffer << data
          end
          if buffer.size == size
            command, content = buffer[0, size].split(/\0/, 2)
            if content.nil?
              raise DecodeError, "NULL character to terminate command is missing"
            end
            return parse(command, content)
          elsif buffer.size > size
            raise DecodeError, "data is large: <#{buffer.size}/#{size}>"
          end
        end
        actual_size = 0
        actual_size = buffer.size if buffer
        expected_size = size || "N/A"
        raise DecodeError, "data is shorten: <#{actual_size}/#{expected_size}>"
      end

      def read_data(server)
        data = nil
        Timeout.timeout(1) do
          begin
            data = server.readpartial(4096)
          rescue EOFError
          end
        end
        data
      end

      def parse(command, content)
        parse_method = "parse_#{command}"
        if self.class.private_method_defined?(parse_method)
          send(parse_method, content)
        else
          raise DecodeError, "unknown command: <#{command}>"
        end
      end

      def parse_status(content)
        content
      end

      def parse_configuration(content)
        hash = Hash.from_xml(content)
        unless hash.has_key?("configuration")
          raise "root element isn't <configuration>"
        end
        configuration = hash["configuration"]

        last_modified = configuration["last_modified"]
        if last_modified
          configuration["last_modified"] = Time.at(last_modified.to_i)
        end

        normalize_container_element!(configuration, "milters", "milter")
        milters = configuration["milters"]
        if milters
          milters.each do |milter|
            normalize_container_element!(milter,
                                         "applicable_conditions",
                                         "applicable_condition")
          end
        end
        normalize_container_element!(configuration,
                                     "applicable_conditions",
                                     "applicable_condition")

        configuration
      end

      def normalize_container_element!(parent, container_name, element_name)
        elements = parent[container_name]
        if elements
          elements = elements[element_name] || []
          elements = [elements] unless elements.is_a?(Array)
          if elements.empty?
            parent.delete(container_name)
          else
            parent[container_name] = elements
          end
        end
      end

      def parse_success(content)
        content
      end

      def parse_error(content)
        raise Error.new(content)
      end
    end
  end
end
