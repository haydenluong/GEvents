# GEvents

GEvents is a C++ search engine for campus events. You enter keywords, pick your interests and a date range, and get back events ranked by relevance. Event keywords are indexed with two structures written from scratch, a Trie and a hash table, and the program compares them on insertion time, search time, and memory as the dataset grows past 100,000 events.

Group 75: Khang "Hayden" Luong, Raymond Shi, Phuc Nguyen

## Building

You need CMake 3.16 or newer and a C++17 compiler.

```
cmake -S . -B build
cmake --build build
```

This produces the `gevents` executable in `build/` (on Visual Studio generators, `build/Debug/` or `build/Release/`).

## Project Layout

| Folder | Contents |
| :--- | :--- |
| `include/` | Headers |
| `src/` | Source files |
| `tools/` | Python dataset generator |
| `data/` | Generated datasets (not committed) |

## Dataset

The dataset is generated locally and is never committed. Generate it with a fixed seed so everyone works with the same file:

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

The Trie also provides `autocomplete(prefix, limit)`, which returns up to `limit` keywords starting with `prefix` in alphabetical order.
