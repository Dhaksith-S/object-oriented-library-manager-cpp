"""Capture unedited stdout from two real application processes, using disposable data.

Run from the repository root: python tools/capture_execution.py build-release/library_manager.exe
"""
import pathlib
import subprocess
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
exe = pathlib.Path(sys.argv[1]).resolve()
sample = root / "data" / "sample_library.csv"
sessions = [
    ("First run: invalid input, import, catalog, registration, search, issue, availability, return, history, save",
     "abc\n11\n2\n4\nTaylor Example\ntaylor@example.invalid\n3\ntitle\nCLOCKWORK\n6\n1\n9\n10\n1\n7\n11\n9\n12\n13\n14\n"),
    ("Restart: reload, member record, restored availability, loan history, summary",
     "5\n10\n1\n9\n13\n14\n"),
]
output = ["AUTHENTIC APPLICATION EXECUTION\n",
          "Captured by tools/capture_execution.py from two separate executable processes.\n",
          "The temporary database is removed after capture. Contacts are fictional.\n",
          "Session labels and supplied stdin are capture metadata; OUTPUT blocks are unedited program output.\n"]
with tempfile.TemporaryDirectory(prefix="library-evidence-") as folder:
    db = pathlib.Path(folder) / "library.csv"
    for label, text in sessions:
        result = subprocess.run([str(exe), "--db", str(db), "--sample", str(sample)],
                                input=text, text=True, capture_output=True, timeout=30, check=True)
        output += [f"\n=== {label} ===\n", "INPUT (one answer per line):\n", text,
                   "OUTPUT:\n", result.stdout, "STDERR:\n", result.stderr or "(empty)\n",
                   f"EXIT CODE: {result.returncode}\n"]
target = root / "docs" / "sample-execution.txt"
target.parent.mkdir(parents=True, exist_ok=True)
target.write_text("".join(output), encoding="utf-8")
print("Captured docs/sample-execution.txt (two successful processes).")
