import pathlib
import subprocess
import sys
import tempfile


def run(path, commands):
    result = subprocess.run(
        [sys.argv[1], str(path)], input=commands, text=True,
        capture_output=True, timeout=30,
    )
    assert result.returncode == 0, result.stderr
    return result.stdout


with tempfile.TemporaryDirectory() as folder:
    data = pathlib.Path(folder) / "events.tsv"
    header = "id\ttitle\tdate\tlocation\tcategory\tdescription\tkeywords\n"
    data.write_text(header +
        "90\tArt club\t2026-11-07\tGallery\tarts\tPaint together\tpaint;learn\n"
        "42\tCoding workshop\t2026-11-05\tLibrary\ttechnology\tBuild something\tcode;learn\n"
        "7\tCoding meetup\t2026-11-06\tUnion\ttechnology\tMeet others\tcode;learn\n")
    search = "2\nCODE learn\ntechnology\ntechnology\n2026-11-05\n2026-11-06\n"
    output = run(data, search + "4\n" + search + "3\nco\n5\n0\n")
    assert output.count("2 matching events") == 2, output
    assert output.count("42 | Coding workshop") == 2, output
    assert output.count("7 | Coding meetup") == 2, output
    assert "Art club | score" not in output, output
    assert "Index: Trie" in output and "code\n" in output, output
    assert "at least 10,000" in output, output
    output = run(data, "2\ncode\n\n\n2026-11-31\n\n0\n")
    assert "Invalid calendar date" in output and "Goodbye." in output, output
    output = run(data, "1\n/no/such/events.tsv\n2\n\narts\n\n\n\n0\n")
    assert "Could not open" in output and "90 | Art club" in output, output
    output = run(pathlib.Path(folder) / "missing.tsv", "2\ninvalid\n0\n")
    assert "Load a dataset first" in output and "Choose a menu option" in output, output
    assert "Goodbye." in run(data, "2\n"), "EOF did not exit cleanly"
    duplicate = pathlib.Path(folder) / "duplicate.tsv"
    duplicate.write_text(header +
        "7\tA\t2026-11-05\tHall\tarts\tA\tpaint\n"
        "7\tB\t2026-11-06\tHall\tarts\tB\tpaint\n")
    assert "Duplicate event ID" in run(duplicate, "0\n")
    pages = pathlib.Path(folder) / "pages.tsv"
    pages.write_text(header + "".join(
        f"{i}\tEvent {i}\t2026-11-05\tHall\tarts\tDetails\tpaint\n"
        for i in range(12)))
    output = run(pages, "2\n\n\n\n\n\ny\n0\n")
    assert "12 matching events" in output and "11 | Event 11" in output, output
    assert "Show more?" in output, output
    print("Console integration tests passed")
