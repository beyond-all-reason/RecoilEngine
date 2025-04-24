# index

{% if types %}
## Types
{% for item in types %}
- [{{ item.name }}]({{ item.file }}){% endfor %}
{% endif %}

{% if modules %}
## Modules
{% for item in modules -%}
- [{{ item.name }}]({{ item.file }})
{% endfor %}
{% endif %}

{% if globals %}
## Globals
{% for item in globals -%}
- [{{ item.name }}]({{ item.file }})
{% endfor %}
{% endif %}