# GEvents

GEvents is a search engine for campus events, written in C++ for our COP3530 Project 2. You type in some keywords, pick the categories you care about and a date range, and it gives you back matching events ranked by how relevant they are.

The main point of the project is comparing two data structures we wrote ourselves: a Trie and a hash table. Both store the same event keywords, and we measure how fast they build, how fast they search, and how much memory they use as the dataset grows past 100,000 events.

Group 75: Khang Luong, Raymond Shi, Phuc Nguyen

## What it can do

- Search events by one or more keywords. An event has to contain all of them to show up.
- Filter by category and by date range.
- Rank results so events matching your interests and happening closer to today come first.
- Autocomplete keywords from a prefix using the Trie.
- Switch between the hash table and the Trie for searching.
- Benchmark both structures at 10k, 25k, 50k, and 100k events.
- Do all of this from either the terminal menu or a small search window.

## Getting started

You'll need CMake 3.16 or newer, a C++17 compiler, and Python 3.

**1. Build it**

```
cmake -S . -B build
cmake --build build
```

This makes `gevents` (the menu), `gevents_benchmark`, and `gevents_backend` (used by the search window) in `build/`. With a Visual Studio generator they end up in `build/Debug/` or `build/Release/` instead.

**2. Generate the dataset**

We don't commit the data, so make it locally. The fixed seed means everyone gets the exact same file:

```
python tools/generate_events.py --count 100000 --seed 42 --out data/events.tsv
```

**3. Run it**

Run everything from the repo root so the default `data/events.tsv` path works.

```
./build/gevents
```

or, for the window:

```
python tools/gevents_ui.py
```

## The terminal menu

You can also pass a dataset path directly: `./build/gevents path/to/events.tsv`.

1. **Load dataset** reads a TSV file into both indexes. If the new file is broken, you keep the dataset you already had. Duplicate IDs and bad dates get rejected.
2. **Search events** asks for keywords, which categories to allow, which categories you're interested in, and a start and end date. Leaving keywords blank browses everything, and leaving a filter blank means no restriction. Results come ten at a time with their scores, and you type `y` to see more.
3. **Autocomplete** shows up to ten keywords that start with your prefix. It always uses the Trie, whichever index is active.
4. **Switch index** flips searching between the hash table (the default) and the Trie.
5. **Run benchmarks** compares both structures on the loaded data, averaged over three runs, and can save the results to a CSV. You need at least 10,000 events loaded.
0. **Exit** closes the program.

## The search window

If you'd rather not use the terminal, the window can do everything the menu does, using the same C++ code underneath, so you get the same results. It needs Tkinter, which comes with the normal Python installer, and matplotlib (`pip install matplotlib`) for the benchmark charts. To start with a different dataset, add `--data path/to/events.tsv`.

- **File → Load dataset** picks a different TSV file. If the file is broken, you keep the dataset you already had.
- Type keywords and press Enter or click **Search**. Leaving the box blank browses everything.
- Trie suggestions show up while you type, and clicking one fills in the word. With an empty box, the list shows the first keywords alphabetically.
- Use the **Index** buttons to switch between the hash table and the Trie. Above the results you'll see how many events matched and how long the index lookup took.
- Tick categories to only show those, or tick interests to rank those categories higher.
- Set a start or end date (YYYY-MM-DD), or leave them blank.
- Results come 100 at a time. Click **Show more** for the next batch, and click a result to see its description and keywords.
- **Run benchmark** compares both structures on the loaded data and opens a table with charts. **Save CSV** writes the same CSV the benchmark program does.
- **File → Exit** closes the window.

The lookup time shown after a search comes from that one search, so expect it to be higher than the averaged numbers from the benchmark.

## Benchmarks and charts

For timing, use a Release build. Debug builds are a lot slower and the numbers aren't meaningful.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/gevents_benchmark --data data/events.tsv --csv data/benchmark.csv --repeats 5
```

The benchmark builds both structures at 10k, 25k, 50k, and 100k events and times insertion and exact-match search on the same queries. It also records memory. The first run is a warm-up and isn't counted, and the rest are averaged.

To turn the CSV into charts (needs matplotlib, `pip install matplotlib`):

```
python tools/plot_benchmark.py
```

The PNGs are saved to `data/charts/`.

## Tests

```
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

These cover the Trie, the hash table, the filters and ranking, and the console menu. The console test runs only when CMake can find Python. You can also run it on its own with `python tests/test_console.py ./build/gevents`.

## How the code is laid out

| Folder | What's in it |
| :--- | :--- |
| `include/` | Headers |
| `src/` | The main program and shared code |
| `bench/` | The benchmark program |
| `ui/` | The backend the search window talks to |
| `tests/` | Tests for the indexes, ranking, and menu |
| `tools/` | Python scripts: dataset generator, charts, search window |
| `docs/` | Our algorithms write-up |
| `data/` | Generated files (not committed) |

## The dataset

`data/events.tsv` has a header row and then one event per line, with fields separated by tabs. No field contains a tab or a newline.

| Field | Format |
| :--- | :--- |
| `id` | A whole number equal to the row number, starting at 0 |
| `title` | Text |
| `date` | `YYYY-MM-DD` |
| `location` | Text |
| `category` | One of `social`, `technology`, `entrepreneurship`, `academic`, `sports`, `arts`, `career`, `volunteering` |
| `description` | Text |
| `keywords` | 3 to 8 lowercase words separated by semicolons, with no spaces inside a keyword |

## How the two structures fit together

The Trie and the hash table both inherit from `Index` in `include/index.h`, so the rest of the program can use either one the same way:

- `insert(keyword, eventId)` adds an event ID to that keyword's list. We insert events in ID order, so the lists stay sorted without us having to sort them.
- `find(keyword)` returns that keyword's event IDs, sorted with no duplicates. An unknown keyword gives you an empty list.
- `memoryBytes()` adds up everything the structure allocated for itself (nodes, bucket arrays, and the capacity of each ID list). It doesn't count the event records, since both structures share those.

The Trie has one extra method, `autocomplete(prefix, limit)`, which returns up to `limit` keywords starting with `prefix`, in alphabetical order.

### Trie

Every node lives in one big vector, and children point to each other by index rather than by pointer. Each node keeps its children sorted by letter, so finding a child is a binary search. The node where a keyword ends holds that keyword's event IDs. Autocomplete walks down to the prefix's node and then visits children in letter order until it has enough words.

### Hash table

`HashTable` uses FNV-1a hashing with separate chaining. It starts with 16 buckets and doubles them before a new keyword would push the load factor over 0.75. When it resizes, it moves the existing nodes over instead of copying them. ID lists stay sorted with no duplicates even if IDs come in out of order. Empty keywords are ignored, the same as in the Trie.

Keywords are case-sensitive, so the query parser lowercases everything before searching. For memory, the table counts its bucket array, its chain nodes, any keyword text stored on the heap, and the capacity of each ID list. It doesn't count the table object itself or temporary vectors returned by `find`. Whether short keywords end up on the heap depends on the compiler, so check this on the machine you run the final comparison on.

## How searching and ranking work

When you search with keywords, the program calls `find` once for each keyword on whichever index is active. It then intersects the sorted ID lists by walking through two of them at a time. The matching IDs are turned back into events and passed to the ranking code in `include/ranking.h`.

The options in `SearchOptions` are:

- `categories`: only allow these categories. Empty means allow all of them.
- `interests`: categories you like. They raise the score but don't hide anything else.
- `startDate` and `endDate`: inclusive bounds in `YYYY-MM-DD`. Empty means no limit.
- `referenceDate`: the date used for the "close to today" bonus. The program sets it to today. Leaving it empty turns the bonus off.

An event has to contain every keyword you searched for. Then it gets a score:

```
score = 3 * matched keywords
      + 2 if its category is one of your interests
      + 1 / (1 + days between the event and the reference date)
```

Higher scores come first. Ties go to the earlier date, then the smaller ID. Past events still show up unless you set a start date. A bad date or a start date after the end date gives you an error instead of results.
