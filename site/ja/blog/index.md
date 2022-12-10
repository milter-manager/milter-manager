---
pagination:
  enabled: true
path_prefix: /ja/
language: ja
country: JP
---
<div id="posts">
  {% for post in paginator.posts %}
    <div class="post">
      <span class="date"><a href="{{ post.url | relative_url }}" title="{{ post.title | xml_escape }} ({{post.date | date: '%Y-%m-%d' }})">{{ post.date | date: "%Y-%m-%d" }}</a></span>
      <div class="excerpt">{{ post.excerpt }}</div>
      <p>
        <a class="btn more" href="{{ post.url | relative_url }}">
          Read more
        </a>
      </p>
    </div>
  {% endfor %}
</div>

{% if paginator.total_pages > 1 %}
<ul class="pagination">
  {% if paginator.next_page %}
  <li class="page-item">
    <a class="page-link"
       href="{{ paginator.next_page_path | relative_url }}">&lt;</a>
  </li>
  {% endif %}
  <li class="page-item">
    <a class="page-link" href="{{ "/ja/blog/" | relative_url }}">最新記事</a>
  </li>
  {% if paginator.previous_page %}
  <li class="page-item">
    <a class="page-link"
       href="{{ paginator.previous_page_path | relative_url }}">&gt;</a>
  </li>
  {% endif %}
</ul>
{% endif %}
