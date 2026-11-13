import csv
import runpy
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "summarize-presentmon.py"
read_groups = runpy.run_path(str(SCRIPT))["read_groups"]


class PresentMonTest(unittest.TestCase):
    def test_v1_and_default_capture_report_the_same_intervals(self):
        group = ["Application", "ProcessID", "SwapChainAddress", "PresentMode"]
        formats = {
            "v1": ["QPCTime", "msBetweenPresents", "msBetweenDisplayChange", "Dropped"],
            "default": ["TimeInQPC", "MsBetweenPresents", "MsBetweenDisplayChange"],
        }
        rows = [
            (42, 50, 80, 80, 0),
            (42, 100, 10, 16, 0),
            (42, 200, 10, 0, 1),
            (42, 300, 10, 20, 0),
            (17, 300, 55, 55, 0),
            (42, 400, 45, 45, 0),
        ]
        reports = {}
        with tempfile.TemporaryDirectory() as directory:
            for name, fields in formats.items():
                path = Path(directory) / (name + ".csv")
                with path.open("w", newline="", encoding="utf-8-sig") as file:
                    csv_rows = csv.writer(file)
                    csv_rows.writerow(group + fields)
                    for pid, tick, present, display, dropped in rows:
                        values = ["overg.exe", pid, "0x100", "Composed: Flip", tick, present]
                        if name == "v1":
                            values.extend([display, dropped])
                        else:
                            values.append("NA" if dropped else display)
                        csv_rows.writerow(values)
                reports[name] = read_groups(path, 100, 400, 42)
        self.assertEqual(reports["v1"], reports["default"])
        report, = reports["v1"]
        self.assertEqual(report["MsBetweenPresents"]["valid"], 3)
        display = report["MsBetweenDisplayChange"]
        self.assertEqual((display["valid"], display["missing"], display["p50_ms"]), (2, 1, 18))


if __name__ == "__main__":
    unittest.main()
