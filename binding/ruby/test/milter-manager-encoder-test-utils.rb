module MilterManagerEncoderTestUtils
  include MilterEncoderTestUtils

  private
  def packet(mark, content='')
    super(mark + "\0", content)
  end
end
