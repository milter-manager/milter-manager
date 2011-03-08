# Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
#
# Licence: Ruby's

require 'shellwords'

module Shellwords
  unless respond_to?(:split)
    module_function
    def split(line)
      shellwords(line)
    end
  end

  unless respond_to?(:escape)
    module_function
    # backported from ruby 1.8.7
    #
    # Author: Akinori MUSHA <knu@iDaemons.org>
    def escape(str)
      # An empty argument will be skipped, so return empty quotes.
      return "''" if str.empty?

      str = str.dup

      # Process as a single byte sequence because not all shell
      # implementations are multibyte aware.
      str.gsub!(/([^A-Za-z0-9_\-.,:\/@\n])/n, "\\\\\\1")

      # A LF cannot be escaped with a backslash because a backslash + LF
      # combo is regarded as line continuation and simply ignored.
      str.gsub!(/\n/, "'\n'")

      return str
    end
  end

  unless respond_to?(:join)
    module_function
    def join(array)
      array.collect {|str| escape(str)}.join(' ')
    end
  end
end
