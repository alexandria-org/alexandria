
# Search Result Ranking
The base of our search result ranking is the harmonic centrality of a domain harmonic(domain).

```
rank(url, word) = (
    harmonic(domain(url)) +
    sum(domain(url), word, rank(link_source, link_target)) * weight1 +
    sum(url, word, rank(link_source, link_target)) * weight2
  ) * IDF(w)
```

Where sum(domain(url), word, rank(link_source, link_target)) is the sum of the inlinks to the domain with a link text that contains the word. And sum(url, word, rank(link_source, link_target)) is the sum of all inlinks to the specific url. weight1 and weight2 are constant weights.

rank(link_source, link_target) is a ranking function for links that depends on the harmonic centrality for the domains.

```
rank(link_source, link_target) = max(harmonic(domain(link_source)) - harmonic(domain(link_target)), harmonic(domain(link_source)) / 100)
```

![link rank formula](https://github.com/joscul/alexandria/raw/main/documentation/images/figure_1.png)
