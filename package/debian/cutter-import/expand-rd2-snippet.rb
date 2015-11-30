#!/usr/bin/env ruby

$LOAD_PATH.unshift(File.dirname(__FILE__))
require 'rd2refentry-lib'
require 'rd/rdfmt'
require 'fileutils'

if ARGV.size != 1
  puts "Usage: #{$0} DOCBOOK_XML_DIR"
  exit 1
end

class RD2RefEntrySnippetVisitor < RD::RD2RefEntryVisitor
  def apply_to_DocumentElement(element, contents)
    section_contents = collect_section_contents(contents)
    if section_contents.empty?
      contents.join("\n\n")
    else
      section_contents.join("\n\n").gsub(/<(\/)?refsect(\d)/) do
        "<#{$1}refsect#{$2.to_i.succ}"
      end
    end
  end

  def apply_to_Verbatim(element)
    if /\A#\s*pass-through\s*\z/ =~ (element.content[0] || '')
      element.content[1..-1].join
    else
      super
    end
  end

  def apply_to_String(element)
    element.gsub(/([^<]*)(<.+?>)?/) do
      content = $1
      tag = $2
      h(content).gsub(/&amp;commat;/, '&commat;') + (tag || "")
    end
  end
end

def expand_rd2_snippet(rd_snippet)
  source = "=begin\n#{rd_snippet}\n=end\n"
  tree = RD::RDTree.new(source)
  visitor = RD2RefEntrySnippetVisitor.new
  visitor.visit(tree)
end

def expand_rd2_snippets(file_name)
  content = File.open(file_name) {|input| input.read}
  rd2_snippet_re = /<para>\s*<rd>\s*(.+?)\s*<\/rd>\s*<\/para>/m
  expanded_content = content.gsub(rd2_snippet_re) do
    rd2_snippet = $1
    program_listing_re = /\n.*?<programlisting>(?m:.+?)<\/programlisting>.*?\n/
    rd2_snippet = rd2_snippet.gsub(program_listing_re) do |program_listing|
      "\n  # pass-through" + program_listing.gsub(/\n/, "\n  ")
    end
    rd2_snippet = rd2_snippet.gsub(/<\/para>\n<para>\n/, "\n")
    expand_rd2_snippet(rd2_snippet)
  end
  changed = content != expanded_content
  if changed
    File.open(file_name, "w") {|output| output.print(expanded_content)}
  end
  changed
end

xml_dir_name = ARGV[0]
changed = false
Dir.open(xml_dir_name) do |dir|
  dir.each do |name|
    next if /\.xml\z/ !~ name
    changed = true if expand_rd2_snippets(File.join(xml_dir_name, name))
  end
end

if changed
  FileUtils.touch(File.join(File.dirname(xml_dir_name), "sgml.stamp"))
end
