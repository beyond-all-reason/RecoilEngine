---
layout: default
title: {{ doc.name }}
parent: Lua API
permalink: lua-api/types/{{ doc.name }}
---
{# Use `render_with_liquid: false` when we upgrade to Jekyl >=4.0 #}
{{ "{% raw %}" }}

# class {{ doc.name }}

{% if doc.supers %}
- supers: {{ doc.supers }}
{% endif %}
{% if doc.namespace %}
- namespace: {{ doc.namespace }}
{% endif %}

{% if doc.property.description %}
{{ doc.property.description }}
{% endif %}

{% if doc.property.deprecated %}
@deprecated {{ doc.property.deprecated }}
{% endif %}

{% if doc.property.see %}
@see {{ doc.property.see }}
{% endif %}

{% if doc.property.other %}
{{ doc.property.other }}
{% endif %}
---
{% if doc.methods %}
## methods
---
{% for method in doc.methods %}
### {{ method.name }}
---
{{ method.display }}

{% if method.property.description %}
{{ method.property.description }}
{% endif %}

{% if method.property.deprecated %}
@deprecated {{ method.property.deprecated }}
{% endif %}

{% if method.property.see %}
@see {{ method.property.see }}
{% endif %}

{% if method.property.other %}
{{ method.property.other }}
{% endif %}
{% endfor %}
{% endif %}

{% if doc.fields %}
## fields
---
{% for field in doc.fields %}
### {{ field.name }}
---
{{ field.display }}

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