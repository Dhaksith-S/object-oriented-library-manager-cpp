"""Real subprocess integration checks; no third-party Python dependencies."""
import os
import pathlib
import subprocess
import sys
import tempfile
import unittest

EXE = pathlib.Path(sys.argv[1]).resolve()
SAMPLE = pathlib.Path(sys.argv[2]).resolve()
sys.argv = sys.argv[:1]


class ConsoleTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="library-cli-")
        self.addCleanup(self.temp.cleanup)
        self.db = pathlib.Path(self.temp.name) / "library.csv"

    def run_app(self, text, code=0, db=None, sample=None):
        result = subprocess.run(
            [str(EXE), "--db", str(db or self.db), "--sample", str(sample or SAMPLE)],
            input=text, text=True, capture_output=True, timeout=20,
        )
        self.assertEqual(result.returncode, code, result.stdout + result.stderr)
        return result.stdout + result.stderr

    def test_complete_journey_and_restart(self):
        out = self.run_app("11\n4\nTaylor Example\ntaylor@example.invalid\n3\ntitle\nCLOCKWORK\n6\n1\n9\n10\n1\n8\n7\n11\n9\n12\n13\n14\n")
        for expected in ("Sample data imported", "Member registered. ID: 9", "The Clockwork Orchard",
                         "Loan ID: 11", "Available copies: 2 / 3", "Book returned", "Library records saved",
                         "Returned loans: 7", "Goodbye"):
            self.assertIn(expected, out)
        restarted = self.run_app("5\n10\n1\n9\n13\n14\n")
        self.assertIn("Saved records loaded", restarted)
        self.assertIn("Taylor Example", restarted)
        self.assertIn("Available copies: 3 / 3", restarted)
        self.assertIn("Returned loans: 7", restarted)

    def test_empty_library_and_eof(self):
        out = self.run_app("2\n5\n8\n9\n13\n")
        for expected in ("No books found", "No members registered", "No loans found", "Books: 0", "End of input"):
            self.assertIn(expected, out)

    def test_invalid_input_recovers(self):
        out = self.run_app("abc\n\n-1\n99999999999999999999\n0\n15\n10\n-1\n10\n99\n7\n99\n13\n14\n")
        self.assertGreaterEqual(out.count("Error:"), 9)
        self.assertIn("Unknown book ID", out)
        self.assertIn("Unknown loan ID", out)
        self.assertIn("Books: 0", out)

    def test_add_book_and_registration_validation(self):
        out = self.run_app("1\n42\nA very long title " + "x" * 90 + "\nAn Author\nScience\n2\n2\n"
                           "1\n42\nDuplicate\nAuthor\nGenre\n1\n4\n\ncontact\n"
                           "4\nAlex Example\ncontact@example.invalid\n6\n42\n1\n10\n42\n7\n1\n7\n1\n14\n")
        for expected in ("Book added", "...", "Duplicate book ID", "Text must contain", "Member registered. ID: 1",
                         "Book issued", "Available copies: 1 / 2", "Book returned", "already been returned"):
            self.assertIn(expected, out)

    def test_import_idempotence_and_conflict(self):
        out = self.run_app("11\n11\n13\n7\n7\n11\n13\n14\n")
        self.assertIn("already present; no changes", out)
        self.assertIn("Import conflicts", out)
        self.assertIn("Active loans: 3 | Returned loans: 7", out)

    def test_borrowing_errors_and_limit(self):
        out = self.run_app("11\n6\n1\n8\n6\n1\n99\n6\n99\n1\n6\n15\n1\n"
                           "6\n1\n1\n6\n1\n1\n6\n1\n1\n13\n14\n")
        for expected in ("Member is inactive", "Unknown member ID", "Unknown book ID", "Book unavailable",
                         "borrowing limit reached", "Active loans: 6"):
            self.assertIn(expected, out)

    def test_corrupt_storage_preserved(self):
        self.db.write_text("corrupt\n", encoding="utf-8")
        out = self.run_app("14\n", code=1)
        self.assertIn("Existing files were preserved", out)
        self.assertEqual(self.db.read_text(), "corrupt\n")

    def test_failed_save_rolls_back_memory(self):
        pathlib.Path(str(self.db) + ".tmp").write_text("stale", encoding="utf-8")
        out = self.run_app("4\nExample\ncontact\n5\n14\n")
        self.assertIn("Stale .tmp/.stage", out)
        self.assertIn("No members registered", out)
        self.assertNotIn("Member registered. ID", out)
        self.assertFalse(self.db.exists())

    def test_eof_mid_mutation(self):
        out = self.run_app("1\n1\nTitle\n")
        self.assertIn("End of input", out)
        self.assertFalse(self.db.exists())

    def test_sample_protection_and_bad_import(self):
        out = self.run_app("14\n", db=SAMPLE, code=1)
        self.assertIn("separate from sample", out)
        missing = pathlib.Path(self.temp.name) / "missing.csv"
        out = self.run_app("11\n13\n14\n", sample=missing)
        self.assertIn("does not exist", out)
        self.assertIn("Books: 0", out)

    def test_sample_alias_protected(self):
        source = pathlib.Path(self.temp.name) / "sample.csv"
        source.write_bytes(SAMPLE.read_bytes())
        os.link(source, self.db)
        out = self.run_app("11\n14\n", sample=source, code=1)
        self.assertIn("separate from sample", out)
        self.assertEqual(source.read_bytes(), SAMPLE.read_bytes())

    def test_backup_recovery_in_process(self):
        self.run_app("11\n4\nNew Example\ncontact\n14\n")
        self.db.write_text("broken primary\n", encoding="utf-8")
        out = self.run_app("13\n12\n14\n")
        self.assertIn("WARNING: recovered previous backup", out)
        self.assertIn("Members: 8", out)
        self.assertEqual(pathlib.Path(str(self.db) + ".corrupt").read_text(), "broken primary\n")
        self.assertIn("Saved records loaded", self.run_app("14\n"))

    def test_help_and_bad_arguments(self):
        result = subprocess.run([str(EXE), "--help"], capture_output=True, text=True, timeout=20)
        self.assertEqual(result.returncode, 0)
        self.assertIn("Usage: library_manager", result.stdout)
        result = subprocess.run([str(EXE), "--unknown"], capture_output=True, text=True, timeout=20)
        self.assertEqual(result.returncode, 1)
        self.assertIn("Startup error", result.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=2)
