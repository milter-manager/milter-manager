module MilterManagerEncoderTestUtils
  include MilterManagerTestUtils

  private
  def packet(command, content='')
    pack(command + "\0" + content)
  end

  def pack(content)
    [content.size].pack("N") + content
  end
end
