def md(string)
  string ||= ""
  string = string.gsub('"', '\"').split("\n").join('" "')
  %Q({{< md "#{string}" >}})
end

H_TEMPLATE = ERB.new <<~'EOF'
  <h<%= dom_level %>>
  <%= name %>
  <span id="<%= ref %>" class="hx-absolute -hx-mt-20"></span>
  <a href="#<%= ref %>" class="subheading-anchor" aria-label="Permalink for this section"></a>
  </h<%= dom_level %>>
EOF

def h(dom_level, name, ref)
  dom_level = dom_level + 2

  H_TEMPLATE.result(binding)
end

class Member < OpenStruct
  @@enum_template = ERB.new <<~'EOF'
    <%= h(dom_level, full_name + ' <em class="hx-text-sm">(' + type.to_s + (" overload #{overload_index}" if overload_index).to_s + ')</em>', ref) %>

    <%= md description %>

    <% if deprecated %>

    Deprecated<%= ": #{deprecation_reason}" if deprecation_reason %>
    <% end %>
    <% if see %>

    See <%= see %>
    <% end %>

    <dl><%= generate_fields() %></dl>
  EOF

  @@function_template = ERB.new <<~'EOF'
    <%= h(dom_level, full_name + ' <em class="hx-text-sm">(' + type.to_s + (" overload #{overload_index}" if overload_index).to_s + ')</em>', ref) %>
    <%= md description %>
    <% if deprecated %>

    Deprecated<%= ": #{deprecation_reason}" if deprecation_reason %>
    <% end %>
    <% if see %>

    See <%= see %>
    <% end %>

    ```lua
  <%= full_name %>(<%= params.map {|p| p["name"] == "..." ? "[#{p["typ"]}, ]*" : p["typ"] }.join(", ") unless params.empty?
  %>) -> <% if returns.empty? %>nil<% else %><%= returns.map {|p| p["name"] == "..." ? "[#{p["typ"]}, ]*" : p["typ"] }.join(", ")  %><% end %>
    ```

    <% if not params.empty? %>
    <%= h(dom_level + 1, "Parameters", "#{ref}_arguments") %>

    <dl class="pl-1">
    <% params.each do |param| %>
    <dt><code><%= param["typeref"] %></code><a class="pl-025" href="#<%= param["ref"] %>"><%= param["name"] %></a><span id="<%= param["ref"] %>" class="hx-absolute -hx-mt-20"></span></dt>
    <dd><%= md(param["desc"]) %></dd>
    <% end %>
    </dl>
    <% end %>

    <% if not returns.empty? %>
    <%= h(dom_level + 1, "Returns", "#{ref}_returns") %>

    <dl class="pl-1">
    <% returns.each do |return_value| %>
    <dt><code><%= return_value["typeref"] %></code><a class="pl-025" href="#<%= return_value["ref"] %>"><%= return_value["name"] %></a><span id="<%= return_value["ref"] %>" class="hx-absolute -hx-mt-20"></span></dt>
    <dd><%= md(return_value["desc"]) %></dd>
    <% end %>
    </dl>
    <% end %>
  EOF

  @@table_template = ERB.new <<~'EOF'
    <%= h(dom_level, full_name + ' <em class="hx-text-sm">(' + type.to_s + (" overload #{overload_index}" if overload_index).to_s + ')</em>', ref) %>

    <%= md description %>
    <% if deprecated %>

    Deprecated<%= ": #{deprecation_reason}" if deprecation_reason %>
    <% end %>
    <% if see %>

    See <%= see %>
    <% end %>

    <% if not children.empty? %>
    <%= h(dom_level + 1, "Members", "#{ref}_members") %>

    <%= generate_children() %>
    <% end %>

    <% if not fields.empty? %>
    <%= h(dom_level + 1, "Fields", "#{ref}_fields") %>

    <dl class="pl-1"><%= generate_fields() %></dl>
    <% end %>
  EOF

  @@class_template = ERB.new <<~'EOF'
    <%= h(dom_level, full_name + ' <em class="hx-text-sm">(' + type.to_s + (" overload #{overload_index}" if overload_index).to_s + ')</em>', ref) %>

    <% if not bases.empty? %>
    Inherits <% bases.each do |base| %><code><%= base %></code> <% end %>
    <% end %>


    <%= md description %>
    <% if deprecated %>

    Deprecated<%= ": #{deprecation_reason}" if deprecation_reason %>
    <% end %>
    <% if see %>

    See <%= see %>
    <% end %>

    <% if not children.empty? %>
    <%= h(dom_level + 1, "Members", "#{ref}_members") %>

    <%= generate_children() %>
    <% end %>

    <% if not fields.empty? %>
    <%= h(dom_level + 1, "Fields", "#{ref}_fields") %>

    <dl class="pl-1"><%= generate_fields() %></dl>
    <% end %>
  EOF

  @@global_template = ERB.new <<~'EOF'
    <%= h(dom_level, full_name + ' <em class="hx-text-sm">(' + type.to_s + (" overload #{overload_index}" if overload_index).to_s + ')</em>', ref) %>

    <%= md description %>
    <% if deprecated %>

    Deprecated<%= ": #{deprecation_reason}" if deprecation_reason %>
    <% end %>
    <% if see %>

    See <%= see %>
    <% end %>

    <% if not children.empty? %>
    <%= h(dom_level + 1, "Members", "#{ref}_members") %>

    <%= generate_children() %>
    <% end %>

    <% if not fields.empty? %>
    <%= h(dom_level + 1, "Fields", "#{ref}_fields") %>

    <dl class="pl-1"><%= generate_fields() %></dl>
    <% end %>
  EOF

  @@alias_template = ERB.new <<~'EOF'
    <dt><a href="#<%= ref %>"><%= full_name %></a><span id="<%= ref %>" class="hx-absolute -hx-mt-20"></span></dt>
    <dd><code><%= literal || typeref %></code></dd>
  EOF

  @@dunno_template = ERB.new <<~'EOF'
    <%= h(dom_level, full_name + ' <em class="hx-text-sm">(' + type.to_s + (" overload #{overload_index}" if overload_index).to_s + ')</em>', ref) %>

    <%= md description %>
    <% if deprecated %>

    Deprecated<%= ": #{deprecation_reason}" if deprecation_reason %>
    <% end %>
    <% if see %>

    See <%= see %>
    <% end %>
  EOF

  @@definition_template = ERB.new <<~'EOF'
    <% if type != :alias && !custom["helper"] %>
    <div class="hx-grid hx-gap-2 grid-cols-2 mb-1 mt-1 align-baseline">
      <h3 id="<%= ref %>-heading" data-notoc="">
        <a href="#<%= ref %>"><%= name %> <% if overload_index %> <em class="hx-text-sm">(overload <%= overload_index %>)</em> <% end %></a>
      </h3>
      <p><%= md(summary) %></p>
    </div>

    <% if type != :enum && !custom["no_toc_contents"] %>
    <dl class="hx-grid hx-gap-2 grid-cols-2 mt-1 pl-1">
      <%= generate_children(:definition_field) %>
    </dl>
    <% end %>
    <% end %>
  EOF

  @@definition_field_template = ERB.new <<~'EOF'
    <dt class="col-span-1 mt-0 mb-0"><a href="#<%= ref %>"><%= name %></a></dd>
    <dd class="col-span-1 mt-0 mb-0 p-inline-start-none"><% if overload_index %><em class="hx-text-sm">(overload <%= overload_index %>)</em> <% end %><%= md(summary) %></dd>
  EOF

  @@field_template = ERB.new <<~'EOF'
    <dt><code><%= literal || typeref %></code> <a href="#<%= ref %>"><%= name %></a><span id="<%= ref %>" class="hx-absolute -hx-mt-20"></span></dt>
    <dd><%= md(description) %></dd>
  EOF

  @@templates = {
    enum: @@enum_template,
    dunno: @@dunno_template,
    field: @@field_template,
    table: @@table_template,
    definition: @@definition_template,
    definition_field: @@definition_field_template,
    class: @@class_template,
    function: @@function_template,
    alias: @@alias_template,
  }
end
