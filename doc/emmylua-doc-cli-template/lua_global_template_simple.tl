---
<<<<<<< HEAD
layout: default
title: {{ doc.name }}
parent: Lua API
permalink: lua-api/globals/{{ doc.name }}
---
{# Use `render_with_liquid: false` when we upgrade to Jekyl >=4.0 #}
{{ "{% raw %}" }}
=======
title: "{{ doc.name }}"
permalink: "docs/lua-api/{{ doc.name }}"
---
>>>>>>> 9338b0b8cf (added site)

# global {{ doc.name }}

---
{% if doc.display %}
{{ doc.display }}
{% endif %}

{% if doc.property.description %}
{{ doc.property.description }}
{% endif %}
<<<<<<< HEAD

{{ "{% endraw %}" }}
=======
>>>>>>> 9338b0b8cf (added site)
