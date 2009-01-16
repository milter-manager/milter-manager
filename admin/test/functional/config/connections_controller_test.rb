require 'test_helper'

class Config::ConnectionsControllerTest < ActionController::TestCase
  def test_index
    assert_raise(ActionController::UnknownAction) do
      get(:index)
    end
  end

  def test_new
    assert_raise(ActionController::UnknownAction) do
      get(:new)
    end
  end

  def test_create
    assert_raise(ActionController::UnknownAction) do
      post(:create, :config_connection => {:spec => "inet:2929@localhost"})
    end
  end

  def test_show
    get(:show, :id => connections(:default).id)
    assert_response(:success)
  end

  def test_edit
    get(:edit, :id => connections(:default).id)
    assert_response(:success)
  end

  def test_update
    put(:update,
        :id => connections(:default).id,
        :config_connection => {
          :spec_type => "inet",
          :spec_port => "2929",
          :spec_host => "localhost"
        })

    connection = assigns(:connection)
    assert_redirected_to(connection)
    assert_equal("inet:2929@localhost", connection.spec)
  end

  def test_destroy
    assert_raise(ActionController::UnknownAction) do
      delete(:destroy, :id => connections(:default).id)
    end
  end
end
