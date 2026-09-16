#!/usr/bin/env python3
"""Cross check the sensekit-stats CSV against numpy.

The point of this script is not to be clever, it is to be an independent second
implementation: if the C++ numbers and the numpy numbers disagree, one of the two
is wrong and the disagreement is the lesson.

    python tools/cross_check_numpy.py D:\\datasets\\UCI-HAR\\train\\Inertial Signals\\body_acc_x_train.txt --columns 0,1,2
"""

from __future__ import annotations

import argparse
import sys

try:
    import numpy as np
except ImportError:  # pragma: no cover - the message is the feature
    sys.exit("numpy is required: python -m pip install numpy")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", help="numeric text file to read")
    parser.add_argument("--delimiter", choices=["auto", "comma", "whitespace"], default="auto")
    parser.add_argument("--header", choices=["auto", "none", "first-row"], default="auto")
    parser.add_argument("--columns", default="all", help="all, indices like 0,2 or names")
    parser.add_argument("--ddof", type=int, choices=[0, 1], default=0)
    return parser.parse_args()


def content_lines(path: str) -> list[str]:
    with open(path, encoding="utf-8-sig") as handle:
        return [
            line.strip()
            for line in handle
            if line.strip() and not line.strip().startswith("#")
        ]


def resolve_delimiter(lines: list[str], requested: str) -> str | None:
    if requested == "comma":
        return ","
    if requested == "whitespace":
        return None
    return "," if "," in lines[0] else None


def resolve_skiprows(lines: list[str], delimiter: str | None, requested: str) -> int:
    if requested == "first-row":
        return 1
    if requested == "none":
        return 0
    fields = lines[0].split(delimiter) if delimiter else lines[0].split()
    try:
        [float(field) for field in fields]
    except ValueError:
        return 1
    return 0


def select_columns(data: np.ndarray, names: list[str], spec: str) -> list[tuple[int, str]]:
    if spec == "all":
        return list(enumerate(names))
    selection: list[tuple[int, str]] = []
    for token in spec.split(","):
        token = token.strip()
        if not token:
            sys.exit("--columns contains an empty entry")
        if token.isdigit():
            index = int(token)
            if index >= len(names):
                sys.exit(f"--columns index {index} is out of range (0..{len(names) - 1})")
            selection.append((index, names[index]))
        elif token in names:
            selection.append((names.index(token), token))
        else:
            sys.exit(f"--columns name '{token}' is not a column of this table")
    return selection


def main() -> int:
    args = parse_args()

    lines = content_lines(args.path)
    if not lines:
        sys.exit(f"{args.path}: no data rows found")

    delimiter = resolve_delimiter(lines, args.delimiter)
    skiprows = resolve_skiprows(lines, delimiter, args.header)

    data = np.loadtxt(args.path, delimiter=delimiter, comments="#", skiprows=skiprows, ndmin=2)
    if data.size == 0:
        sys.exit(f"{args.path}: no data rows found")

    column_count = data.shape[1]
    if skiprows == 1:
        header_fields = lines[0].split(delimiter) if delimiter else lines[0].split()
        names = [field.strip() for field in header_fields]
    else:
        names = [f"column_{index}" for index in range(column_count)]

    print("column,count,mean,variance,rms")
    for index, name in select_columns(data, names, args.columns):
        values = data[:, index]
        mean = float(np.mean(values))
        variance = float(np.var(values, ddof=args.ddof))
        rms = float(np.sqrt(np.mean(values**2)))
        print(f"{name},{values.size},{mean:.10g},{variance:.10g},{rms:.10g}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
