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

