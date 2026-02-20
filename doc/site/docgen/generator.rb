require 'ostruct'
require 'erb'
require 'json'
require 'set'

require_relative "templates.rb"

def a(name)
  %Q(<a href="##{name}">#{name}</a>)
end

class Member
  @@other_matcher = Regexp.compile('\@\*(\w+)\*( (.*))?')
  @@refs = Set.new

  def self.refs; @@refs; end

  @@known_templates = Set.new(@@templates.keys)

  @@top_level = Set.new(["Spring", "Callins", "SyncedCallins", "UnsyncedCallins", "gl", "GL", "RmlUi"])

  def initialize(attributes, parent = nil)
    super(attributes)

    initialize_attributes(parent)
  end

  def generate_sidebar_entry()
    entries = if fields.empty? or children.empty?
      ""
    else
      "{name = 'Fields', link = '##{ref}_fields', ref = '#{ref}_fields'}, "
    end

    entries += children.map(&:generate_sidebar_entry).join(', ')

    "{name = '#{name}', link = '##{ref}', ref = '#{ref}', entries = [#{entries}]}"
  end

  def generate_children(member_type = nil)
    if member_type
      self.children.map {|c| c.generate(member_type) }.join("\n")
    else
      self.children.map(&:generate).join("\n")
    end
  end

  def generate_fields(member_type = nil)
    self.fields.map(&:generate).join("\n")
  end

  def generate(internal_type = nil)
    internal_type = internal_type || self.type

    # weird stuff, for some reason @global becomes @field
    if internal_type == :field && parent.nil?
      internal_type = :table
    end

    template = @@templates.include?(internal_type) ? internal_type : :dunno

    @@templates[template].result(binding)
  end

  def push_to_refs()
    self.ref = overload_index ? "#{full_name}-#{overload_index}" : full_name

    if @@refs.include?(self.ref)
      puts "> WARNING: Trying to add already existent global reference #{self.ref}"
    end

    @@refs.add(self.ref)
  end

  @@deep_type_ref_matcher = Regexp.new('[^\s\(\)\[\]\|\?&;]+')

  def self.replace_deep_type_refs(value)
    return value if not value&.is_a?(String)

    value
      .gsub("<", "&lt;").gsub(">", "&gt;") # escape < > to html
      .gsub(@@deep_type_ref_matcher) do |match|
        @@refs.include?(match) ? a(match) : match
      end
  end

  @@ref_matcher = Regexp.new("`([^`]*)`")

  def self.replace_refs(value)
    return value if not value&.is_a?(String)

    matches = value.scan(@@ref_matcher)

    return value if matches.empty?

    value = value.dup

    matches.map(&:first)
           .select {|m| @@refs.include?(m) }.uniq
           .each {|m| value.gsub!("`#{m}`", a(m)) }
    value
      .gsub(@@ref_matcher) do |match|
        "<code>#{self.replace_deep_type_refs($1)}</code>"
      end
  end

  def process_refs()
    self.see = a(see) if @@refs.include?(see)

    self.description = Member.replace_refs(self.description)

    params&.each do |p|
      p["typeref"] = Member.replace_deep_type_refs(p["typ"])
      p["ref"] = "#{full_name}-params.#{p["name"]}"
      p["desc"] = Member.replace_refs(p["desc"])
    end

    returns&.each do |p|
      p["typeref"] = Member.replace_deep_type_refs(p["typ"])
      p["ref"] = "#{full_name}-returns.#{p["name"]}"
      p["desc"] = Member.replace_refs(p["desc"])
    end

    self.typeref = Member.replace_deep_type_refs(self.typ)

    if self.description
      self.description&.gsub!("##", "####")
      segments = self.description.strip.split("\n")

      self.summary = segments.slice!(0)&.strip
      self.short_description = segments.join("\n")
    end

    children.each(&:process_refs)
    fields.each(&:process_refs)
  end

  def process_members()
    (self.members || [])
            .map {|m| Member.new(m, self)}
            .group_by(&:name)
            .flat_map do |(name, ms)|

      ms.each_with_index do |m, i|
        m.overload_index = i if i > 0
        m.push_to_refs()
      end

      ms
    end
  end

  def self.compare(m1, m2)
    helper1 = m1.custom["x_helper"]
    helper2 = m2.custom["x_helper"]

    # Tagged helpers should be at the utmost bottom
    if helper1 && helper1 == helper2
      0
    elsif helper1
      1
    elsif helper2
      -1
    elsif m1.type == m2.type
      m1.full_name <=> m2.full_name
    end
  end

  private

  def initialize_attributes(parent)
    self.parent = parent
    self.level = parent ? parent.level + 1 : 0
    self.dom_level = self.level

    if parent
      separator = is_meth ? ":" : "."
      self.full_name = "#{parent.full_name}#{separator}#{self.name}"

      # classes and tables are nested under members
      if parent.type == :class || parent.type == :table
        self.dom_level = self.dom_level + 1
      end
    else
      self.full_name = self.name || "noname?"

      # global has no children to check for duplicates/overloads
      push_to_refs()
    end

    self.type = self.type.to_sym
    self.type = :function if self.type == :fn

    self.members = self.members || []
    self.children = []
    self.fields = []

    if not self.members.empty?
      aliases, non_aliases = self.members.map {|m| Member.new(m, self) }
                                .partition {|m| m.type == :alias }

      self.members = non_aliases
      self.aliases = aliases

      processed_members = process_members()# .sort { |m1, m2| Member.compare(m1, m2) }

      fields, children = processed_members.partition {|c| c.type == :field }

      self.fields = fields
      self.children = children
    end

    extract_custom_tags()
  end

  def extract_custom_tags()
    other = self.other

    if !other
      self.custom = {}
      self.section = nil

      return
    end

    custom = other.split("\n\n").reduce({}) do |acc, el|
      name, _, value = el.match(@@other_matcher).captures

      if name
        acc[name] = value || true
      end

      acc
    end

    self.custom = custom
    self.section = custom["section"]
  end
end

class Generator
  @@top_level = ["Spring", "Callins", "SyncedCallins", "UnsyncedCallins", "gl", "GL", "RmlUi"]
    .each_with_index
    .reduce({}) {|acc, (el, i)| acc[el] = i + 1; acc }

  def self.compare_members(m1, m2)
    # helpers to the bottom
    helper1 = m1.custom["x_helper"]
    helper2 = m2.custom["x_helper"]

    if helper1 && (helper1 == helper2)
      return 0
    elsif helper1
      return 1
    elsif helper2
      return -1
    end

    # top levels to the top
    include1 = @@top_level[m1.name]
    include2 = @@top_level[m2.name]

    if include1 && include2
      include1 <=> include2
    elsif include1
      -1
    elsif include2
      1
    elsif m1.type == m2.type
      m1.name <=> m2.name
    else
      m1.type <=> m2.type
    end
  end

  def initialize(data_file)
    data = JSON.load_file(data_file)

    aliases, non_aliases = (data["globals"] + data["types"])
      .map {|g| Member.new(g) }
      .partition{|g| g.type == :alias }

    @aliases = aliases

    @globals = non_aliases.sort { |m1, m2| Generator.compare_members(m1, m2) }

    (@globals + @aliases).each(&:process_refs)
  end

  def generate
    # ## Table of Contents
    #
    # #{@globals.map { |el| el.generate(:definition) }.join("\n")}
    entries = @globals.map(&:generate_sidebar_entry).join(',')

    <<~EOF
      +++
      title = "Lua API"
      type = "docs"
      layout = "lua_api"
      [params]
      entries = [#{entries}]
      +++

      ## Overview

      We list here all globals available among all different lua environments
      within Recoil. Some types listed here exist only as helpers for aliasing
      parameters and returns.

      The documentation pages are still a work in progress.

      #{@globals.map(&:generate).join("\n")}

      #{"## Aliases\n\n" + \
        "<dl>#{@aliases.map(&:generate).join("\n")}</dl>" \
        if not @aliases.empty?}
    EOF
  end
end

def usage_error()
  puts <<~'EOF'
    Usage:

    --doc: An emmylua-doc-cli compatible documentation file
    --out: A file path to be written

    Example:

    --doc=doc.json --out=doc.md
  EOF

  exit(1)
end

def run()
  args = Hash[ARGV.join(' ').scan(/--?([^=\s]+)(?:=(\S+))?/)]
  args = OpenStruct.new(args)

  if not args.doc or not File.exist?(args.doc)
    puts "Error: Invalid or non existent documentation file #{args.doc}\n\n"

    usage_error()
  end

  if not args.out
    puts "Error: Invalid or non existent out path #{args.out}\n\n"

    usage_error()
  end

  generator = Generator.new(args.doc)

  generated = generator.generate()

  puts "Writing to #{File.expand_path(args.out)}"

  File.open(args.out, "w") { |f| f.write(generated) }
end

run()
