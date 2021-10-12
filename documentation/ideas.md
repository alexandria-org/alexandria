#Similar words
To handle similar words (saluhall, saluhallen) we should create a hashtable with similar words and as an additional index create "saluhall+" by combining our existing indexes of saluhall, saluhallen, saluhallarna etc. into one additional index.

#Autocomplete
We should base our autocomplete on the most common words in titles of documents before and after each word. For example "Uppsala" could suggest "Uppsala kommun", "Uppsala universitet" and "Destination Uppsala" based on the search results.
