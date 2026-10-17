import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest

from run import summarize


class ReportTests(unittest.TestCase):
    def report(self, rows):
        with tempfile.TemporaryDirectory() as directory, contextlib.redirect_stdout(io.StringIO()):
            output = Path(directory)
            result = summarize("\n".join("AIMFROM_AUDIT\t" + row for row in rows), output)
            self.assertEqual(json.loads((output / "summary.json").read_text()), result)
            return result

    def rows(self, first="same", second="different"):
        return ["begin\t2\tfixture\tfixture", "unit\ttank\tcandidate", "unit\tbeam\tunaffected-type",
                f"weapon\ttank\t2\tgun\tCannon\tCOB\t1\t{first}\tquery\taim",
                f"weapon\ttank\t2\tgun\tCannon\tCOB\t30\t{second}\tquery\taim", "complete\t2"]

    def test_any_difference_wins(self):
        self.assertEqual(self.report(self.rows())["weapon_categories"], {"different": 1})

    def test_unknown_is_not_same(self):
        self.assertEqual(self.report(self.rows("same", "unknown"))["weapon_categories"], {"unknown": 1})

    def test_both_same(self):
        self.assertEqual(self.report(self.rows("same", "same"))["weapon_categories"], {"same": 1})

    def test_incomplete_run_rejected(self):
        with self.assertRaises(AssertionError):
            self.report(self.rows()[:-1])

    def test_missing_sample_rejected(self):
        rows = self.rows()
        del rows[3]
        with self.assertRaises(AssertionError):
            self.report(rows)

    def test_creation_failure_rejected(self):
        rows = self.rows()
        rows[1] = "unit\ttank\tcreate-failed"
        with self.assertRaises(AssertionError):
            self.report(rows)


if __name__ == "__main__":
    unittest.main()
