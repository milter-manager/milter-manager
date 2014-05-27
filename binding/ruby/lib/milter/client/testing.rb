module Milter
  module TestClient
    def spawn_milter(milter_path, *args)
      default_options = {
        :out => "/dev/null",
        :err => "/dev/null",
      }
      if args.last.is_a?(Hash)
        options = args.pop
      else
        options = {}
      end
      options = default_options.merge(options)

      if options[:env]
        env = options.delete(:env)
      else
        env = {}
      end

      port = options.delete(:port) || 20025
      host = options.delete(:host) || "localhost"
      spec = "inet:#{port}@#{host}"

      pid = spawn(env,
                  RbConfig.ruby,
                  milter_path,
                  "--connection-spec=#{spec}",
                  *args,
                  options)

      start = Time.now
      Process.detach(pid)
      loop do
        break if Time.now - start > 5
        begin
          TCPSocket.new(host, port)
          break
        rescue SystemCallError
        end
      end
      pid
    end

    def kill_milter(pid)
      Process.kill(:KILL, pid)
    rescue SystemCallError
    end
  end
end
