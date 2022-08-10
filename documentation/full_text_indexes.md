# The alexandria full text index

A full text index in its simplest form is a hash map from an integer word id k to a list of document ids v.

We built two kinds of data structures called ```index``` and ```counted_index``` they are very similar so we will start by explaining the similar parts and then explain the differences and why we needed two.

## Data layout

...
