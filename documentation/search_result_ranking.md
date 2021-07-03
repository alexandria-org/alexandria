# Search Result Ranking
The base of our search result ranking is the harmonic centrality of a domain H(d).

R(u, w) = H(d) + SUM(w, R(L1, L2))

Where SUM(w, R(L1, L2)) is the sum of the inlinks to the domain with a link text that contains the word w. R(L1, L2) is a ranking function for links that depends on the harmonic centrality for domain 1 and 2.

## Ranking function for domain links
The ranking function R(L1, L2) gives a score for links from domain 1 to domain 2 depends on the difference in harmonic centrality of domain 1 and domain 2.

```
R(L1, L2) = 
1000 if H(D1) <= H(D2)
(H(D1) - H(D2))/10 if H(D1) - H(D2) < 10 000 000
1 000 000 otherwise
```
