# GEvents

GEvents is a C++ search engine for campus events. You enter keywords, pick your interests and a date range, and get back events ranked by relevance. Event keywords are indexed with two structures written from scratch, a Trie and a hash table, and the program compares them on insertion time, search time, and memory as the dataset grows past 100,000 events.

Group 75: Khang "Hayden" Luong, Raymond Shi, Phuc Nguyen

## Current Status

The custom hash table, category/date filters, and relevance ranking are implemented as C++ components with automated tests. The executable currently prints `GEvents`; the console menu, dataset loader/generator, Trie, and benchmark harness are not implemented in this checkout yet.

## Building

You need CMake 3.16 or newer and a C++17 compiler.

```
cmake -S . -B build
cmake --build build
```

This produces the `gevents` executable in `build/` (on Visual Studio generators, `build/Debug/` or `build/Release/`).

Run the executable:

```
./build/gevents
```

On a Visual Studio generator, run `build/Debug/gevents.exe` instead.

Build and run the component tests:

```
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

## Project Layout

| Folder | Contents |
| :--- | :--- |
| `include/` | Headers |
| `src/` | Source files |
| `tests/` | Hash table, filter, and ranking tests |
| `docs/` | Algorithms report draft |
| `tools/` | Planned Python dataset generator |
| `data/` | Generated datasets (not committed) |

## Dataset

The full dataset will be generated locally and excluded from Git. The generator is not present yet. Once implemented, the planned command uses a fixed seed so everyone works with the same file:

```
python tools/generate_events.py --count 100000 --seed 42 --out data/events.tsv
```

### File format

`data/events.tsv` has one header row, then one event per line. Fields are separated by a tab and appear in this order:

| Field | Format |
| :--- | :--- |
| `id` | Integer equal to the row number, starting at 0 |
| `title` | Text |
| `date` | `YYYY-MM-DD` |
| `location` | Text |
| `category` | One of `social`, `technology`, `entrepreneurship`, `academic`, `sports`, `arts`, `career`, `volunteering` |
| `description` | Text |
| `keywords` | 3 to 8 lowercase words separated by semicolons, with no spaces inside a keyword |

No field contains a tab or a newline.

## Index Interface

Both structures derive from `Index` in `include/index.h`:

- `insert(keyword, eventId)` adds an event ID to a keyword's posting list. Events are inserted in ascending ID order.
- `find(keyword)` returns the event IDs for a keyword in ascending order with no duplicates, or an empty vector if the keyword is unknown.
- `memoryBytes()` counts every byte the structure allocates itself (nodes, bucket arrays, and the capacity of each ID vector), not the event records.

The planned Trie also provides `autocomplete(prefix, limit)`, which returns up to `limit` keywords starting with `prefix` in alphabetical order.

## Hash Table

`HashTable` in `include/hash_table.h` implements `Index` using FNV-1a hashing and separate chaining. It starts with 16 buckets by default and doubles the bucket count before a new distinct keyword would exceed a 0.75 load factor. Rehashing relinks existing nodes. Posting lists stay sorted and duplicate IDs are ignored, including when callers insert IDs out of order. Empty keywords are ignored, matching the Trie. Keywords are case-sensitive; query parsing should normalize them to lowercase before indexing or searching.

`memoryBytes()` counts the allocated bucket array, chain nodes, heap keyword storage, and posting-list capacity. It excludes the table object itself, event records, allocator metadata, and temporary vectors returned by `find`. Heap keyword accounting detects whether character storage is inside the string object; verify that convention on the compiler used for the final comparison. Both indexes must use the same accounting boundaries.

## Filters and Ranking

`include/ranking.h` exposes `SearchOptions`, `filterEvents`, and `rankEvents`.

- `categories`: allowed categories; an empty vector allows all categories.
- `startDate` and `endDate`: inclusive bounds in `YYYY-MM-DD`; an empty bound is unrestricted.
- `interests`: category preferences that increase relevance without excluding other categories.
- `referenceDate`: date used for the proximity bonus; empty disables the bonus. The menu should supply today's local date explicitly.

Invalid calendar dates or reversed bounds throw `std::invalid_argument`. Category and keyword comparisons are case-sensitive. Ranking enforces all distinct query keywords, then computes:

```
score = 3 * matched distinct keywords
      + 2 * (event category is in interests)
      + 1 / (1 + absolute days from referenceDate)
```

The final term is zero when no reference date is supplied. Scores sort from highest to lowest, with ties resolved by earlier date and then smaller event ID. An empty query browses all events that pass the filters. Past events remain eligible unless the caller sets a start date.

Example integration after the dataset loader produces `events`:

```cpp
#include "hash_table.h"
#include "ranking.h"

HashTable index;
for (const auto& event : events) {
    for (const auto& keyword : event.keywords) {
        index.insert(keyword, event.id);
    }
}

SearchOptions options;
options.interests = {"technology"};
options.startDate = "2026-11-05";
options.referenceDate = "2026-11-05";
auto ranked = rankEvents(events, {"code", "learn"}, options);
```

For indexed search, the query parser should call `find` for each keyword, intersect the sorted posting lists, resolve those IDs to event records, and pass those candidate records to `rankEvents`. Event IDs are identifiers, not necessarily vector offsets. The example passes all events to show the ranking API independently of the pending query parser.
