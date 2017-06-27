class TestRspamdProxyDetector < Test::Unit::TestCase
  data(default: ["*:11332", "inet:11332@localhost"],
       host: ["mail.example.com:11332", "inet:11332@mail.example.com"])
  test "detect" do |(bind_socket, expected)|
    detector = ::Milter::Manager::RspamdProxyDetector.new
    mock(File).executable?("/usr/bin/rspamadm") { true }
    mock(detector).__double_definition_create__.call("`".to_sym, "/usr/bin/rspamadm configdump --json") do
      {
        "worker" => [
          {
            "rspamd_proxy" => {
              "bind_socket" => bind_socket,
              "milter" => true
            }
          }
        ]
      }.to_json
    end

    assert_equal(expected, detector.detect)
  end
end
