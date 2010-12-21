module MilterParseTestUtils
  include MilterTestUtils

  private
  def create_input(source)
    input = StringIO.new(source)
    def input.path
      nil
    end
    input
  end

end
