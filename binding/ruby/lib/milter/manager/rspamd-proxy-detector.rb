require "json"

module Milter::Manager
  class RspamdProxyDetector
    def initialize(rspamadm_path="/usr/bin/rspamadm")
      @rspamadm_path = rspamadm_path
    end

    def detect
      return unless @rspamadm_path
      return unless File.executable?(@rspamadm_path)

      stdout, _stderr, wait_status =
                       Milter::CommandRunner.run(@rspamadm_path,
                                                 "configdump",
                                                 "--json")
      if wait_status.zero?
        config = JSON.parse(stdout.strip)
      else
        config = {}
      end
      workers = config["worker"]
      return unless workers
      rspamd_proxy = workers.detect do |worker|
        worker.key?("rspamd_proxy") && worker["rspamd_proxy"]["milter"]
      end
      host, port = rspamd_proxy["rspamd_proxy"]["bind_socket"].split(":", 2)
      if host == "*"
        "inet:#{port}@localhost"
      else
        "inet:#{port}@#{host}"
      end
    end
  end
end
