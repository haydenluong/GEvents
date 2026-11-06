import argparse
import csv
import os
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

SURFACE = "#fcfcfb"
PRIMARY_INK = "#0b0b0b"
SECONDARY_INK = "#52514e"
MUTED_INK = "#898781"
GRID = "#e1e0d9"

SERIES = {
    "Trie": {"color": "#2a78d6", "marker": "o", "label": "Trie"},
    "HashTable": {"color": "#eb6834", "marker": "s", "label": "Hash table"},
}

METRICS = [
    ("insert_ms", "Insertion time", "Time to build (ms)", 1.0, "insert_time.png"),
    ("search_us", "Exact-match search time", "Time per query (µs)", 1.0, "search_time.png"),
    ("memory_bytes", "Memory used", "Memory (MB)", 1024 * 1024, "memory.png"),
]


def parse_arguments():
    parser = argparse.ArgumentParser(description="Plot GEvents benchmark results.")
    parser.add_argument("--csv", default="data/benchmark.csv")
    parser.add_argument("--out", default="data/charts")
    return parser.parse_args()


def read_results(path):
    results = {}
    with open(path, newline="", encoding="utf-8") as file:
        for row in csv.DictReader(file):
            results.setdefault(row["structure"], []).append(row)
    for rows in results.values():
        rows.sort(key=lambda row: int(row["events"]))
    return results


def thousands(value, _position):
    return f"{int(value / 1000)}k"


def style_axes(axes, title, y_label):
    axes.set_facecolor(SURFACE)
    axes.set_title(title, loc="left", color=PRIMARY_INK, fontsize=12, fontweight="bold", pad=12)
    axes.set_xlabel("Events indexed", color=SECONDARY_INK)
    axes.set_ylabel(y_label, color=SECONDARY_INK)
    axes.xaxis.set_major_formatter(FuncFormatter(thousands))
    axes.tick_params(colors=MUTED_INK, labelcolor=SECONDARY_INK, length=0)
    axes.grid(axis="y", color=GRID, linewidth=0.8)
    axes.set_axisbelow(True)
    axes.set_ylim(bottom=0)
    for side in ("top", "right", "left"):
        axes.spines[side].set_visible(False)
    axes.spines["bottom"].set_color(GRID)


def spread_labels(ends, min_gap):
    ends.sort(key=lambda end: end["value"])
    for lower, upper in zip(ends, ends[1:]):
        if upper["label_y"] - lower["label_y"] < min_gap:
            middle = (lower["value"] + upper["value"]) / 2
            lower["label_y"] = middle - min_gap / 2
            upper["label_y"] = middle + min_gap / 2
    return ends


def plot_metric(axes, results, column, divisor):
    ends = []
    for structure, style in SERIES.items():
        rows = results.get(structure)
        if not rows:
            continue
        events = [int(row["events"]) for row in rows]
        values = [float(row[column]) / divisor for row in rows]
        axes.plot(events, values, color=style["color"], linewidth=2, marker=style["marker"],
                  markersize=8, markeredgecolor=SURFACE, markeredgewidth=2, label=style["label"])
        ends.append({"x": events[-1], "value": values[-1], "label_y": values[-1], "label": style["label"]})

    axes.set_ylim(bottom=0)
    top = max(end["value"] for end in ends)
    for end in spread_labels(ends, top * 0.07):
        axes.annotate(end["label"], (end["x"], end["label_y"]), xytext=(10, 0),
                      textcoords="offset points", va="center", color=SECONDARY_INK, fontsize=9)
    axes.set_xticks(sorted({int(row["events"]) for rows in results.values() for row in rows}))
    axes.margins(x=0.14)


def add_legend(axes):
    legend = axes.legend(loc="upper left", frameon=False, labelcolor=SECONDARY_INK)
    return legend


def save_single_charts(results, out_dir):
    for column, title, y_label, divisor, filename in METRICS:
        figure, axes = plt.subplots(figsize=(6.4, 4), facecolor=SURFACE)
        plot_metric(axes, results, column, divisor)
        style_axes(axes, title, y_label)
        add_legend(axes)
        figure.tight_layout()
        figure.savefig(os.path.join(out_dir, filename), dpi=200, facecolor=SURFACE)
        plt.close(figure)


def save_combined_chart(results, out_dir):
    figure, all_axes = plt.subplots(1, len(METRICS), figsize=(15, 4.2), facecolor=SURFACE)
    for axes, (column, title, y_label, divisor, _filename) in zip(all_axes, METRICS):
        plot_metric(axes, results, column, divisor)
        style_axes(axes, title, y_label)
    add_legend(all_axes[0])
    figure.tight_layout()
    figure.savefig(os.path.join(out_dir, "benchmark_summary.png"), dpi=200, facecolor=SURFACE)
    plt.close(figure)


def main():
    arguments = parse_arguments()
    if not os.path.exists(arguments.csv):
        sys.exit("Could not find " + arguments.csv + ". Run gevents_benchmark first.")

    results = read_results(arguments.csv)
    missing = [name for name in SERIES if name not in results]
    if missing:
        sys.exit("No rows for " + ", ".join(missing) + " in " + arguments.csv)

    os.makedirs(arguments.out, exist_ok=True)
    save_single_charts(results, arguments.out)
    save_combined_chart(results, arguments.out)
    print("Saved charts to", arguments.out)


if __name__ == "__main__":
    main()
