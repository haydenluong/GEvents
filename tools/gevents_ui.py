import argparse
import csv
import os
import subprocess
import sys
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CATEGORIES = ["social", "technology", "entrepreneurship", "academic",
              "sports", "arts", "career", "volunteering"]
RESULT_COLUMNS = [("Score", 60), ("Date", 90), ("Title", 320), ("Category", 120), ("Location", 200)]
BENCHMARK_COLUMNS = [("Events", 80), ("Structure", 100), ("Insert (ms)", 100),
                     ("Search (µs/query)", 130), ("Memory (MB)", 100)]
CSV_HEADER = ["structure", "events", "queries", "insert_ms", "search_us", "memory_bytes", "matches"]
SERIES = {
    "Trie": {"color": "#2a78d6", "marker": "o", "label": "Trie"},
    "HashTable": {"color": "#eb6834", "marker": "s", "label": "Hash table"},
}


def parse_arguments():
    parser = argparse.ArgumentParser(description="Search GEvents in a window.")
    parser.add_argument("--data", default=os.path.join(ROOT, "data", "events.tsv"))
    return parser.parse_args()


def find_backend():
    name = "gevents_backend.exe" if os.name == "nt" else "gevents_backend"
    for folder in ["build", os.path.join("build", "Release"), os.path.join("build", "Debug")]:
        path = os.path.join(ROOT, folder, name)
        if os.path.exists(path):
            return path
    sys.exit("Could not find gevents_backend. Build the project with CMake first.")


class Backend:
    def __init__(self, data_path):
        self.data_path = data_path
        self.process = subprocess.Popen([find_backend(), data_path], stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE, text=True, encoding="utf-8")
        status, _, detail = self.process.stdout.readline().strip().partition("\t")
        if status != "ready":
            self.process.wait()
            raise RuntimeError(detail)
        self.event_count = int(detail)

    def request(self, *fields):
        self.process.stdin.write("\t".join(fields) + "\n")
        self.process.stdin.flush()
        lines = []
        for line in self.process.stdout:
            line = line.rstrip("\n")
            if line == "done":
                break
            lines.append(line.split("\t"))
        if lines and lines[0][0] == "error":
            raise ValueError(lines[0][1])
        return lines

    def close(self):
        self.process.stdin.close()
        self.process.wait()


class SearchWindow:
    def __init__(self, root, backend):
        self.root = root
        self.backend = backend
        self.details = {}
        self.last_search = None
        self.shown = 0
        self.benchmark_running = False
        self.benchmark_rows = None
        self.benchmark_error = None
        self.index_choice = tk.StringVar(value="hash")
        self.keywords = tk.StringVar()
        self.start_date = tk.StringVar()
        self.end_date = tk.StringVar()
        self.status = tk.StringVar()
        self.allowed = {name: tk.BooleanVar() for name in CATEGORIES}
        self.interests = {name: tk.BooleanVar() for name in CATEGORIES}

        root.title("GEvents")
        root.geometry("1000x700")
        self.build_menu()
        frame = ttk.Frame(root, padding=12)
        frame.pack(fill="both", expand=True)
        self.build_search_row(frame)
        self.build_filters(frame)
        ttk.Label(frame, textvariable=self.status).pack(anchor="w", pady=(8, 4))
        self.build_results(frame)
        self.clear()

    def build_menu(self):
        menu_bar = tk.Menu(self.root)
        file_menu = tk.Menu(menu_bar, tearoff=False)
        file_menu.add_command(label="Load dataset...", command=self.load_dataset)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.root.destroy)
        menu_bar.add_cascade(label="File", menu=file_menu)
        self.root.config(menu=menu_bar)

    def build_search_row(self, parent):
        row = ttk.Frame(parent)
        row.pack(fill="x")
        row.columnconfigure(0, weight=1)

        ttk.Label(row, text="Keywords").grid(row=0, column=0, sticky="w")
        entry = ttk.Entry(row, textvariable=self.keywords)
        entry.grid(row=1, column=0, sticky="we")
        entry.bind("<KeyRelease>", self.on_key)
        entry.bind("<Return>", lambda event: self.search())
        entry.focus()
        ttk.Button(row, text="Search", command=self.search).grid(row=1, column=1, padx=6)
        ttk.Button(row, text="Clear", command=self.clear).grid(row=1, column=2)

        index_box = ttk.LabelFrame(row, text="Index", padding=(8, 2))
        index_box.grid(row=0, column=3, rowspan=2, padx=(16, 0), sticky="n")
        ttk.Radiobutton(index_box, text="Hash table", value="hash",
                        variable=self.index_choice).pack(anchor="w")
        ttk.Radiobutton(index_box, text="Trie", value="trie",
                        variable=self.index_choice).pack(anchor="w")

        ttk.Label(row, text="Suggestions (Trie autocomplete)").grid(row=2, column=0, sticky="w",
                                                                    pady=(6, 0))
        self.suggestions = tk.Listbox(row, height=4, activestyle="none")
        self.suggestions.grid(row=3, column=0, sticky="we")
        self.suggestions.bind("<<ListboxSelect>>", self.use_suggestion)
        self.benchmark_button = ttk.Button(row, text="Run benchmark", command=self.start_benchmark)
        self.benchmark_button.grid(row=3, column=1, columnspan=2, sticky="nwe", padx=(6, 0))

    def build_filters(self, parent):
        filters = ttk.Frame(parent)
        filters.pack(fill="x", pady=(10, 0))
        rows = [("Only these categories", self.allowed), ("Interests (rank higher)", self.interests)]
        for row, (title, choices) in enumerate(rows):
            ttk.Label(filters, text=title).grid(row=row, column=0, sticky="w", padx=(0, 8))
            for column, name in enumerate(CATEGORIES, start=1):
                box = ttk.Checkbutton(filters, text=name, variable=choices[name])
                box.grid(row=row, column=column, sticky="w")

        dates = ttk.Frame(parent)
        dates.pack(fill="x", pady=(6, 0))
        ttk.Label(dates, text="Start date").pack(side="left")
        ttk.Entry(dates, textvariable=self.start_date, width=12).pack(side="left", padx=(4, 12))
        ttk.Label(dates, text="End date").pack(side="left")
        ttk.Entry(dates, textvariable=self.end_date, width=12).pack(side="left", padx=(4, 8))
        ttk.Label(dates, text="YYYY-MM-DD, blank for no limit").pack(side="left")

    def build_results(self, parent):
        table_frame = ttk.Frame(parent)
        table_frame.pack(fill="both", expand=True)
        self.table = make_table(table_frame, RESULT_COLUMNS, "w")
        scrollbar = ttk.Scrollbar(table_frame, orient="vertical", command=self.table.yview)
        self.table.configure(yscrollcommand=scrollbar.set)
        self.table.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
        self.table.bind("<<TreeviewSelect>>", self.show_details)

        self.more_button = ttk.Button(parent, text="Show more", command=self.show_more)
        self.more_button.pack(anchor="e", pady=(6, 0))
        self.detail_text = tk.Text(parent, height=4, wrap="word", state="disabled")
        self.detail_text.pack(fill="x", pady=(6, 0))

    def load_dataset(self):
        if self.benchmark_running:
            return
        path = filedialog.askopenfilename(title="Load dataset", initialdir=os.path.join(ROOT, "data"),
                                          filetypes=[("Event files", "*.tsv"), ("All files", "*.*")])
        if not path:
            return
        self.status.set("Loading " + os.path.basename(path) + "...")
        self.root.update_idletasks()
        try:
            loaded = Backend(path)
        except RuntimeError as error:
            messagebox.showerror("GEvents", "Could not load " + path + "\n" + str(error) +
                                 "\nThe current dataset is still loaded.")
            self.show_loaded()
            return
        self.backend.close()
        self.backend = loaded
        self.clear()

    def show_loaded(self):
        name = os.path.basename(self.backend.data_path)
        self.status.set(f"Loaded {self.backend.event_count:,} events from {name} into both indexes")

    def search(self):
        if self.benchmark_running:
            return
        self.last_search = ["search", self.index_choice.get(), self.keywords.get(),
                            checked(self.allowed), checked(self.interests),
                            self.start_date.get().strip(), self.end_date.get().strip()]
        self.empty_results()
        self.show_more()

    def show_more(self):
        if self.benchmark_running:
            return
        try:
            lines = self.backend.request(*self.last_search, str(self.shown))
        except ValueError as error:
            messagebox.showerror("GEvents", str(error))
            return
        total = int(lines[0][1])
        lookup = float(lines[0][2])
        for fields in lines[1:]:
            self.add_row(fields)
        self.shown += len(lines) - 1

        if self.shown < total:
            self.more_button.config(state="normal")
        else:
            self.more_button.config(state="disabled")

        if total == 0:
            self.status.set("No events matched. Try a keyword from the suggestions.")
        elif self.last_search[2].strip():
            name = "Trie" if self.last_search[1] == "trie" else "Hash table"
            self.status.set(f"{total:,} matching events | {name} lookup took {lookup:.1f} µs"
                            f" | showing {self.shown:,}")
        else:
            self.status.set(f"{total:,} matching events | showing {self.shown:,}")

    def add_row(self, fields):
        event_id, title, date, category, location, score, description, keywords = fields[1:]
        self.table.insert("", "end", iid=event_id, values=(score, date, title, category, location))
        self.details[event_id] = title + "\n" + description + "\nKeywords: " + keywords

    def empty_results(self):
        self.table.delete(*self.table.get_children())
        self.details = {}
        self.shown = 0
        self.set_details("")

    def show_details(self, event):
        selected = self.table.selection()
        if selected:
            self.set_details(self.details[selected[0]])

    def set_details(self, text):
        self.detail_text.config(state="normal")
        self.detail_text.delete("1.0", "end")
        self.detail_text.insert("1.0", text)
        self.detail_text.config(state="disabled")

    def on_key(self, event):
        if event.keysym != "Return":
            self.update_suggestions()

    def update_suggestions(self):
        if self.benchmark_running:
            return
        self.suggestions.delete(0, "end")
        text = self.keywords.get()
        if text.endswith(" "):
            return
        for _, word in self.backend.request("suggest", text):
            self.suggestions.insert("end", word)

    def use_suggestion(self, event):
        selected = self.suggestions.curselection()
        if not selected:
            return
        words = self.keywords.get().split()
        if not words:
            words = [""]
        words[-1] = self.suggestions.get(selected[0])
        self.keywords.set(" ".join(words) + " ")
        self.suggestions.delete(0, "end")

    def start_benchmark(self):
        self.benchmark_running = True
        self.benchmark_button.config(state="disabled")
        self.root.config(cursor="watch")
        self.status.set("Running the benchmark at 10k, 25k, 50k, and 100k events...")
        self.benchmark_rows = None
        self.benchmark_error = None
        # runs on its own thread so the window does not freeze while it works
        threading.Thread(target=self.run_benchmark, daemon=True).start()
        self.root.after(200, self.check_benchmark)

    def run_benchmark(self):
        try:
            self.benchmark_rows = [line[1:] for line in self.backend.request("benchmark")]
        except ValueError as error:
            self.benchmark_error = str(error)

    def check_benchmark(self):
        if self.benchmark_rows is None and self.benchmark_error is None:
            self.root.after(200, self.check_benchmark)
            return
        self.benchmark_running = False
        self.benchmark_button.config(state="normal")
        self.root.config(cursor="")
        if self.benchmark_error:
            self.status.set("Benchmark failed")
            messagebox.showerror("GEvents", self.benchmark_error)
            return
        self.status.set("Benchmark finished, averaged over 3 runs")
        self.show_benchmark()

    def show_benchmark(self):
        window = tk.Toplevel(self.root)
        window.title("Trie vs hash table")
        table = make_table(window, BENCHMARK_COLUMNS, "e")
        table.config(height=len(self.benchmark_rows))
        for structure, events, queries, insert_ms, search_us, memory, matches in self.benchmark_rows:
            table.insert("", "end", values=(f"{int(events):,}", SERIES[structure]["label"],
                                            f"{float(insert_ms):.2f}", f"{float(search_us):.3f}",
                                            f"{int(memory) / (1024 * 1024):.2f}"))
        table.pack(fill="x", padx=12, pady=(12, 4))

        canvas = FigureCanvasTkAgg(benchmark_figure(self.benchmark_rows), master=window)
        canvas.draw()
        canvas.get_tk_widget().pack(fill="both", expand=True, padx=12)

        footer = ttk.Frame(window)
        footer.pack(fill="x", padx=12, pady=(4, 12))
        ttk.Label(footer, text="Same events and queries for both, averaged over 3 runs.").pack(side="left")
        ttk.Button(footer, text="Save CSV...", command=self.save_benchmark).pack(side="right")

    def save_benchmark(self):
        path = filedialog.asksaveasfilename(title="Save benchmark CSV", initialfile="benchmark.csv",
                                            initialdir=os.path.join(ROOT, "data"),
                                            defaultextension=".csv")
        if not path:
            return
        with open(path, "w", newline="", encoding="utf-8") as file:
            writer = csv.writer(file)
            writer.writerow(CSV_HEADER)
            writer.writerows(self.benchmark_rows)
        messagebox.showinfo("GEvents", "Saved " + path)

    def clear(self):
        self.keywords.set("")
        self.start_date.set("")
        self.end_date.set("")
        for name in CATEGORIES:
            self.allowed[name].set(False)
            self.interests[name].set(False)
        self.empty_results()
        self.more_button.config(state="disabled")
        self.show_loaded()
        self.update_suggestions()


def make_table(parent, columns, anchor):
    names = [heading for heading, width in columns]
    table = ttk.Treeview(parent, columns=names, show="headings")
    for heading, width in columns:
        table.heading(heading, text=heading)
        table.column(heading, width=width, anchor=anchor)
    return table


def checked(choices):
    return " ".join(name for name, chosen in choices.items() if chosen.get())


def benchmark_figure(rows):
    figure = Figure(figsize=(10, 3), dpi=90)
    charts = [(3, "Insertion time (ms)", 1), (4, "Search time (µs/query)", 1),
              (5, "Memory (MB)", 1024 * 1024)]
    for position, (column, title, divisor) in enumerate(charts, start=1):
        axes = figure.add_subplot(1, len(charts), position)
        for structure, style in SERIES.items():
            matching = [row for row in rows if row[0] == structure]
            events = [int(row[1]) for row in matching]
            values = [float(row[column]) / divisor for row in matching]
            axes.plot(events, values, color=style["color"], marker=style["marker"],
                      linewidth=2, label=style["label"])
        axes.set_title(title, fontsize=10, loc="left")
        axes.set_xlabel("Events indexed", fontsize=9)
        axes.set_ylim(bottom=0)
        axes.grid(axis="y", color="#e1e0d9")
        axes.spines["top"].set_visible(False)
        axes.spines["right"].set_visible(False)
    figure.axes[0].legend(frameon=False, fontsize=9)
    figure.tight_layout()
    return figure


def main():
    arguments = parse_arguments()
    try:
        backend = Backend(arguments.data)
    except RuntimeError as error:
        sys.exit("Could not load the dataset: " + str(error))

    root = tk.Tk()
    window = SearchWindow(root, backend)
    root.mainloop()
    window.backend.close()


if __name__ == "__main__":
    main()
