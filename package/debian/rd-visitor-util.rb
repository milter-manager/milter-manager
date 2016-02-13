module RDVisitorUtil
  module_function
  def remove_newline(string)
    content = ""
    if string.respond_to?(:force_encoding)
      string = string.dup.force_encoding("UTF-8")
    end
    lines = string.split(/\r?\n\s*/)
    lines.each_with_index do |line, i|
      line = line.chomp
      next_line = lines[i + 1]
      if next_line and
          (/\A(.)/u =~ next_line && $1.size == 1) and
          (/(.)\z/u =~ line && $1.size == 1)
        content << line + " "
      else
        content << line
      end
    end
    content
  end
end
