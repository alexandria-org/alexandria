
# Search Result Ranking
The base of our search result ranking is the harmonic centrality of a domain H(d).

R(u, w) = (H(d) + SUM_d(w, R(L1, L2)) * w1 + SUM_u(w, R(L1, L2)) * w2 ) * IDF(w)

Where SUM_d(w, R(L1, L2)) is the sum of the inlinks to the domain with a link text that contains the word w. And SUM_u(w, R(L1, L2)) is the sum of all inlinks to the specific URL u. x1 and x2 are constant weights.

R(L1, L2) is a ranking function for links that depends on the harmonic centrality for domain 1 and 2.

## Ranking function for domain links
The ranking function R(L1, L2) gives a score for links from domain 1 to domain 2 depends on the difference in harmonic centrality of domain 1 and domain 2.

```
R(L1, L2) =
1000 if H(D1) <= H(D2)
(H(D1) - H(D2))/10 if H(D1) - H(D2) < 10 000 000
1 000 000 otherwise
```

![Figure 1](https://github.com/joscul/alexandria/raw/main/documentation/images/figure_1.png)

So if a url gets 1000 links from a domain with the same harmonic centrality (HC) or lower it gets a 1M (1 000 000) boost. But if the URL has a domain with a HC of 10M and the linking domain has a HC of 10M it gets a boost of 1M for one single link.

