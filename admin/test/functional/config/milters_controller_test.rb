require 'test_helper'

class Config::MiltersControllerTest < ActionController::TestCase
  include AuthenticatedTestHelper

  def setup
    @thread = nil
  end

  def teardown
    if @thread
      @thread.kill if @thread.join(1).nil?
    end
  end

  def test_index
    get(:index)
    assert_redirected_to(new_session_path)

    login_as(:aaron)
    get(:index)
    assert_response(:success)
    assert_not_nil(assigns(:milters))
  end

  def test_new
    get(:new)
    assert_redirected_to(new_session_path)

    login_as(:aaron)
    get(:new)
    assert_response(:success)
  end

  def test_create
    params = {
      :config_milter => {
        :name => "milter1",
        :connection_spec => "inet:29292@localhost",
      }
    }
    post(:create, params)
    assert_redirected_to(new_session_path)

    login_as(:aaron)
    open_milter_manager_controller do |server|
      @thread = Thread.new do
        client = server.accept
        get_configuration_packet = client.readpartial(4096)
        client.print(packet("configuration",
                            ["<configuration>",
                             "  <dummy>dummy</dummy>",
                             "</configuration>"].join("\n")))
        client.flush

        client = server.accept
        set_configuration_packet = client.readpartial(4096)
        client.print(packet("success"))
        get_status_packet = client.readpartial(4096)
        client.print(packet("status", "FIXME"))
      end
      assert_difference('Config::Milter.count') do
        post(:create, params)
      end
    end
    milter = assigns(:milter)
    assert_redirected_to(milter)
    assert_equal("milter1", milter.name)
  end

  def test_show
    params = {:id => milters(:anti_spam).id}
    get(:show, params)
    assert_redirected_to(new_session_path)

    login_as(:aaron)
    get(:show, params)
    assert_response(:success)
  end

  def test_edit
    params = {:id => milters(:anti_spam).id}
    get(:edit, params)
    assert_redirected_to(new_session_path)

    login_as(:aaron)
    get(:edit, params)
    assert_response(:success)
  end

  def test_update
    params = {
      :id => milters(:anti_spam).id,
      :config_milter => {:name => "changed name"}
    }
    put(:update, params)
    assert_redirected_to(new_session_path)

    login_as(:aaron)
    open_milter_manager_controller do |server|
      @thread = Thread.new do
        client = server.accept
        get_configuration_packet = client.readpartial(4096)
        client.print(packet("configuration",
                            ["<configuration>",
                             "  <dummy>dummy</dummy>",
                             "</configuration>"].join("\n")))
        client.flush

        client = server.accept
        set_configuration_packet = client.readpartial(4096)
        client.print(packet("success"))
        get_status_packet = client.readpartial(4096)
        client.print(packet("status", "FIXME"))
      end
      put(:update, params)
    end

    milter = assigns(:milter)
    assert_redirected_to(milter)
    assert_equal("changed name", milter.name)
  end

  def test_destroy
    params = {:id => milters(:anti_spam).id}
    delete(:destroy, params)
    assert_redirected_to(new_session_path)

    login_as(:aaron)
    assert_difference('Config::Milter.count', -1) do
      delete(:destroy, params)
    end

    assert_redirected_to(config_milters_path)
  end

  private
  def packet(command, content='')
    pack(command + "\0" + content)
  end

  def pack(content)
    [content.size].pack("N") + content
  end

  def open_milter_manager_controller
    spec = ::Config::Connection.default.spec
    spec = ::Milter::Manager::ConnectionSpec.parse(spec)
    host = spec.host || "127.0.0.1"
    port = spec.port
    TCPServer.open(host, port) do |server|
      yield(server)
    end
  end
end
