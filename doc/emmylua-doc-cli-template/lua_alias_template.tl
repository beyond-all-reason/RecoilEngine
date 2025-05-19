---
layout: default
title: {{ doc.name }}
parent: Lua API
permalink: lua-api/types/{{ doc.name }}
---
{# Use `render_with_liquid: false` when we upgrade to Jekyl >=4.0 #}
{{ "{% raw %}" }}

# alias {{ doc.name }}
---
{% if doc.namespace %}
- namespace: {{ doc.namespace }}
{% endif %}

{% if doc.display %}
{{ doc.display }}
{% endif %}

{% if doc.property.description %}
{{ doc.property.description }}
{% endif %}

{{ "{% endraw %}" }}