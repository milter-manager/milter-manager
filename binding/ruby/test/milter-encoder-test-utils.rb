module MilterEncoderTestUtils
  include MilterTestUtils

  private
  def packet(mark, content='')
    pack(mark + content)
  end

  def pack(content)
    [content.size].pack("N") + content
  end
end
