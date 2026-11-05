# Algorithms and Data Structures Draft

## Hash index

The hash table maps each keyword to a sorted vector of event IDs. A custom FNV-1a hash processes the keyword bytes, and modulo selects a bucket. Each bucket owns a singly linked chain, so distinct keywords with the same bucket remain separate. Before a new distinct keyword would push the load factor above 0.75, the table doubles its bucket count and redistributes existing nodes. No standard associative container is used as the index.

Let K be the number of distinct keywords, B the bucket count, L the keyword length, and P the length of a matching posting list. With well-distributed hashes and bounded load factor, locating a keyword takes expected O(L) time. Worst-case collisions require O(KL) string-comparison work. Finding a posting list also copies its P IDs, making expected find time O(L + P). For ascending IDs, posting insertion compares against the last ID in O(1), then appends in amortized O(1). Repeating the last ID returns immediately; other IDs use binary search to locate their position. An out-of-order insertion can shift O(P) IDs. Duplicate IDs do not change the posting list. A resize costs O(B + K + total keyword characters) and preserves the existing posting vectors. Repeated doubling amortizes resizing across new keywords.

Space is O(B + K + total keyword characters + total posting capacity). The reported bytes count actual bucket and posting vector capacities, nodes, and heap string capacity, excluding allocator overhead and event records. This is allocated payload accounting rather than process resident memory.

## Filtering and relevance

The query stage will intersect sorted posting lists to identify events containing every keyword. Intersection of two sorted lists takes O(P1 + P2) time with two advancing positions; the parser and intersection implementation belong to the integration component.

The implemented filter scans candidate events and applies inclusive date bounds plus membership in an allowed category vector. Fixed-width validated dates compare chronologically as strings. Calendar validation handles Gregorian leap years and rejects invalid dates or reversed bounds.

Ranking deduplicates query keywords and verifies every keyword against each candidate's keyword vector. It adds three points per matched distinct keyword, two points for a preferred category, and a proximity bonus of 1 / (1 + absolute day distance). Civil dates are converted to integer day numbers so month lengths and leap days produce correct distances. The reference date is an explicit argument, which makes results reproducible. Among events satisfying an AND query, the keyword term is constant; interest and proximity determine relative relevance.

For N candidates, C category filters, I interests, Q distinct query keywords, and W keywords per event, filtering costs O(N(C + 1)). Ranking costs O(Q log Q + N(QW + C + I + 1) + R log R), where R is the number of retained results. Results sort by descending score, then ascending date and ID. Temporary storage includes copied event records in filtered and ranked vectors; its size depends on the total text and keyword data retained, rather than just the number of events.

## Verification and remaining integration

Automated tests exercise colliding keys, load-factor resizing, 10,000 distinct keyword lookups, duplicate and out-of-order IDs, copied search results, and memory growth. Filter and ranking tests cover AND queries, duplicate keywords, inclusive date boundaries, category exclusions, preference scores, deterministic ties, empty queries, invalid dates, and leap-year distances. These are component checks; the full dataset, menu, Trie comparison, and benchmark results must be verified after integration.

Each member must supply their own reflection on what they learned. Measured timings and final benchmark conclusions belong in the report only after the benchmark runs.
