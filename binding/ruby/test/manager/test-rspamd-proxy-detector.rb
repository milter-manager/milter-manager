class TestRspamdProxyDetector < Test::Unit::TestCase
  data(default: ["*:11332", "inet:11332@localhost"],
       host: ["mail.example.com:11332", "inet:11332@mail.example.com"])
  test "detect" do |(bind_socket, expected)|
    rspamadm = Tempfile.new(["rspamadm", ".sh"])
    FileUtils.chmod(0755, rspamadm.path)
    rspamadm.puts(<<-SHELL)
#!/bin/sh

echo '{'
echo '  "worker": ['
echo '    {'
echo '      "rspamd_proxy": {'
echo '        "bind_socket": "#{bind_socket}",'
echo '        "milter": true'
echo '      }'
echo '    }'
echo '  ]'
echo '}'
    SHELL
    rspamadm.close
    detector = ::Milter::Manager::RspamdProxyDetector.new(rspamadm.path)
    assert_equal(expected, detector.detect)
  end

  test "detect w/ rspamadm_path is nil" do
    detector = ::Milter::Manager::RspamdProxyDetector.new(nil)
    assert_equal(nil, detector.detect)
  end
end
