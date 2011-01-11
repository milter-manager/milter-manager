module MilterEventLoopTestUtils
  include MilterTestUtils

  private
  def create_event_loop
    case ENV["MILTER_EVENT_LOOP_BACKEND"]
    when "glib", nil
      Milter::GLibEventLoop.new
    when "libev"
      Milter::LibevEventLoop.new
    end
  end
end
