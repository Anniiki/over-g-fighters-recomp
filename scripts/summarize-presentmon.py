"""Summarize PresentMon 2.6.0 CSV intervals for each process, swap chain and mode."""
import argparse
import csv
import json
import math
import sys
from collections import defaultdict
from pathlib import Path


GROUP = ("Application", "ProcessID", "SwapChainAddress", "PresentMode")
METRICS = ("MsBetweenPresents", "MsBetweenDisplayChange")


def percentile(values, q):
    """Linear interpolation between adjacent sorted observations."""
    index = (len(values) - 1) * q
    lo = math.floor(index)
    return values[lo] + (values[math.ceil(index)] - values[lo]) * (index - lo)


def summarize(values, rows, include_short):
    values.sort()
    stats = {"valid": len(values), "missing": rows - len(values), "zero": values.count(0.0)}
    for name, q in (("p50", .5), ("p95", .95), ("p99", .99), ("p99_9", .999), ("max", 1)):
        stats[name + "_ms"] = round(percentile(values, q), 6) if values else None
    if include_short:
        stats["percent_below_1_ms"] = (
            round(100 * sum(v < 1 for v in values) / len(values), 4) if values else None
        )
    stats["above_25_ms"] = sum(v > 25 for v in values)
    stats["above_50_ms"] = sum(v > 50 for v in values)
    return stats


def read_groups(path, start_qpc, end_qpc, pid):
    groups = defaultdict(lambda: ([], []))
    counts = defaultdict(int)
    with path.open(encoding="utf-8-sig", newline="") as src:
        reader = csv.DictReader(src)
        columns = set(reader.fieldnames or ())
        legacy = "msBetweenPresents" in columns
        metrics = ("msBetweenPresents", "msBetweenDisplayChange") if legacy else METRICS
        qpc = "QPCTime" if legacy else "TimeInQPC"
        required = set(GROUP + metrics)
        if legacy:
            required.add("Dropped")
        if start_qpc is not None or end_qpc is not None:
            required.add(qpc)
        missing = required - columns
        if missing:
            raise ValueError("Missing CSV columns: " + ", ".join(sorted(missing)))
        for row in reader:
            if None in row or any(row[key] is None for key in required):
                raise ValueError(f"Incomplete or malformed row at line {reader.line_num}")
            if pid is not None and row["ProcessID"].strip() != str(pid):
                continue
            if start_qpc is not None or end_qpc is not None:
                tick = int(row[qpc])
                if (start_qpc is not None and tick < start_qpc) or (end_qpc is not None and tick >= end_qpc):
                    continue
            key = tuple(row[field] for field in GROUP)
            counts[key] += 1
            for field, values in zip(metrics, groups[key]):
                text = row[field].strip()
                if not text or text.upper() == "NA":
                    continue
                # V1 writes a numeric placeholder for a frame that never reached the display.
                if legacy and field == "msBetweenDisplayChange" and row["Dropped"].strip() == "1":
                    continue
                value = float(text)
                if not math.isfinite(value) or value < 0:
                    raise ValueError(f"Invalid {field} at line {reader.line_num}: {text}")
                values.append(value)
    return [
        {**dict(zip(GROUP, key)), "rows": counts[key], **{
            field: summarize(values, counts[key], field == "MsBetweenPresents")
            for field, values in zip(METRICS, groups[key])
        }}
        for key in sorted(groups)
    ]


def capture_warnings(path):
    if path is None:
        return {"checked": False, "warnings": [], "event_loss_warning": None}
    data = path.read_bytes()
    encoding = "utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
    warnings = []
    for line in data.decode(encoding, errors="replace").splitlines():
        if line.lstrip().lower().startswith("warning:"):
            warnings.append(line.strip())
        elif warnings and line[:1].isspace():
            warnings[-1] += " " + line.strip()
    loss = any(any(word in warning.lower() for word in ("overflow", "lost", "dropped events"))
               for warning in warnings)
    return {"checked": True, "warnings": warnings, "event_loss_warning": loss}


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("csv", type=Path)
    cli.add_argument("--stderr", type=Path, help="Saved PresentMon diagnostic log")
    cli.add_argument("--pid", type=int)
    cli.add_argument("--start-qpc", type=int, help="First included QPC tick")
    cli.add_argument("--end-qpc", type=int, help="First excluded QPC tick")
    args = cli.parse_args()
    if args.start_qpc is not None and args.end_qpc is not None and args.end_qpc <= args.start_qpc:
        cli.error("--end-qpc must be greater than --start-qpc")
    try:
        groups = read_groups(args.csv, args.start_qpc, args.end_qpc, args.pid)
        if not groups:
            raise ValueError("No rows match the requested process and interval.")
        report = {"diagnostics": capture_warnings(args.stderr), "groups": groups}
    except (OSError, ValueError, csv.Error) as exc:
        cli.exit(1, f"{cli.prog}: {exc}\n")
    json.dump(report, sys.stdout, indent=2, allow_nan=False)
    print()


if __name__ == "__main__":
    main()
