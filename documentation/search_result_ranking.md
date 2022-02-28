
# Search Result Ranking

This document describes how search results are indexed and ranked.

## Input
Input to our indexer is a sequence of deduplicated urls with the following data.
```
{
    url: "https://www.example.com/",
    title: "Example Page",
    meta_description: "",
    h1: "Example Domain",
    text: "This domain is for use in illustrative examples in documents. You may use this domain in literature without prior coordination or asking for permission. More information..."
}
```

## 1. Domain level
Each url is added with the url hash as key. The tokens are deduplicated

```
domain_score = expm1(5 * link.m_score) + 0.1;
url_score = expm1(10 * link.m_score) + 0.1;
```
