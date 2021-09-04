# Api Response Format

This is a description of the endpoints available on a node.

### Perform search
```
curl http://node0002.alexandria.org/?q=the%20beatles
{
  "status":	"success",
  "time_ms":	35.876,
  "total_found":	245436,
  "total_links_found":	4092,
  "links_handled":	674,
  "link_domain_matches":	18059,
  "link_url_matches":	589,
  "results":	[{
    "url":	"https://www.example.com/",
    "title":	"Example dot com",
    "snippet":	"Lorem ipsum dolor esit",
    "score":	182.51408386230469,
    "domain_hash":	"2892282071861106665",
    "url_hash":	"2892281418178079567"
  }]
}
```

### Perform link search
```
curl http://node0002.alexandria.org/?l=the%20beatles
{
  "status":	"success",
  "time_ms":	35.876,
  "total_found":	245436,
  "results":	[{
    "source_url":	"https://www.example.com/",
    "target_url":	"https://www.alexandria.org/",
    "link_text":	"This is a great search engine",
    "score":	182.51408386230469,
    "link_hash":	"2892282071861106665"
  }]
}
```

### Perform url lookup
```
curl http://node0002.alexandria.org/?u=https://www.example.org/
{
  "status":	"success",
  "time_ms":	35.876,
  "response":	"[DATA]"
}
```

### Fetch information about search result
```
curl http://node0002.alexandria.org/?s=example%20query
{
  "status":	"success",
  "time_ms":	13.984,
  "index":	{
    "total":	980770801,
    "words":	{
      "example":	0.0080152416772448342,
      "query":	0.0017581304401006531
    }
  },
  "link_index":	{
    "total":	472012858,
    "words":	{
      "example":	0.000581251114985516,
      "query":	6.3595725182554242e-05
    }
  }
}
```

### Fetch status of the node.
```
curl http://node0002.alexandria.org/status
{
  "status":	"success",
  "time_ms":	13.984,
  "total_disk_space": 89374934876,
  "avail_disk_space": 83975235,
  "avail_disk_percent": 0.0832,
  "index":	{
    "items":	980770801,
    "full_text_disk_used": 973295875,
    "full_text_disk_percent": 0.5423,
    "hash_table_disk_used": 839265,
    "hash_table_disk_percent": 0.05423
  },
  "link_index":	{
    "items":	980770801,
    "full_text_disk_used": 973295875,
    "full_text_disk_percent": 0.2423,
    "hash_table_disk_used": 839265,
    "hash_table_disk_percent": 0.0423
  }
}
```

### Combined api response (api.alexandria.org)
```
curl https://api.alexandria.org/?q=the%20beatles&p=1
{
  "status":	"success",
  "time_ms":	35.876,
  "total_found":	245436,
  "page_max": 10,
  "results":	[{
    "url":	"https://www.example.com/",
    "display_url": "https://www.example.com/",
    "title":	"Example dot com",
    "snippet":	"Lorem ipsum dolor esit",
    "score":	182.51408386230469,
    "domain_hash":	"2892282071861106665",
    "url_hash":	"2892281418178079567",
    "exact_match": 1,
    "phrase_match": 1,
    "year": 3300,
    "is_old": 0,
    "is_subdomain": 0,
    "domain": "www.example.com"
  },
  ...
  ]
}
```
