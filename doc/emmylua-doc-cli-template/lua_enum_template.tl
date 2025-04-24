---
layout: default
title: {{ doc.name }}
parent: Lua API
permalink: lua-api/types/{{ doc.name }}
---
{# Use `render_with_liquid: false` when we upgrade to Jekyl >=4.0 #}
{{ "{% raw %}" }}

# enum {{ doc.name }}
---
{% if doc.namespace %}
- namespace: {{ doc.namespace }}
{% endif %}

{% if doc.description %}
{{ doc.description }}
{% endif %}
{% if doc.fields %}
{% for field in doc.fields -%}

### {{ field.name }}

```lua
{{ doc.name }}.{{ field.name }} = {{ field.display }}
```
{% if field.property.description %}
{{ field.property.description }}
{% endif %}

{% if field.property.deprecated %}
@deprecated {{ field.property.deprecated }}
{% endif %}

{% if field.property.see %}
@see {{ field.property.see }}
{% endif %}

{% if field.property.other %}
{{ field.property.other }}
{% endif %}

{% endfor %}
{% endif %}

{{ "{% endraw %}" }}