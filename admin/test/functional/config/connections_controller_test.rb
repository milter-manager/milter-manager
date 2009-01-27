require 'test_helper'

class Config::ConnectionsControllerTest < ActionController::TestCase
  include AuthenticatedTestHelper

  def test_index
    get(:index)
    assert_redirected_to(new_session_path)

    login_as(:aaron)
    assert_raise(ActionController::UnknownAction) do
      get(:index)
    end
  end

  def test_new
    get(:new)
    assert_redirected_to(new_session_path)

    login_as(:aaron)
    assert_raise(ActionController::UnknownAction) do
      get(:new)
    end
  end

  def test_create
    params = {:config_connection => {:spec => "inet:2929@localhost"}}
    post(:create, params)
    assert_redirected_to(new_session_path)

    login_as(:aaron)
    assert_raise(ActionController::UnknownAction) do
      post(:create, params)
    end
  end

  def test_show
    get(:show, :id => connections(:default).id)
    assert_redirected_to(new_session_path)

    login_as(:aaron)
    get(:show, :id => connections(:default).id)
    assert_response(:success)
  end

  def test_edit
    get(:edit, :id => connections(:default).id)
    assert_redirected_to(new_session_path)

    login_as(:aaron)
    get(:edit, :id => connections(:default).id)
    assert_response(:success)
  end

  def test_update
    params = {
      :id => connections(:default).id,
      :config_connection => {
        :spec_type => "inet",
        :spec_port => "2929",
        :spec_host => "localhost"
      }}
    put(:update, params)
    assert_redirected_to(new_session_path)

    login_as(:aaron)
    put(:update, params)

    connection = assigns(:connection)
    assert_redirected_to(connection)
    assert_equal("inet:2929@localhost", connection.spec)
  end

  def test_destroy
    params = {:id => connections(:default).id}
    delete(:destroy, params)
    assert_redirected_to(new_session_path)

    login_as(:aaron)
    assert_raise(ActionController::UnknownAction) do
      delete(:destroy, params)
    end
  end
end
