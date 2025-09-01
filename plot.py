#!/usr/bin/env python3
"""
plot.py — read tracker.csv and save a spending chart as a PNG.

Usage:
  python3 plot.py daily        # sums by date -> spending_by_date.png
  python3 plot.py category     # sums by category -> spending_by_category.png
"""

import csv
import sys
from collections import defaultdict, OrderedDict
from datetime import datetime
import matplotlib.pyplot as plt  # do not set styles or colors

CSV_FILE = "tracker.csv"

def read_rows():
    rows = []
    with open(CSV_FILE, newline="", encoding="utf-8") as f:
        r = csv.DictReader(f)
        for row in r:
            # expected columns: date,category,description,amount
            try:
                amt = float(row["amount"])
            except (KeyError, ValueError):
                continue
            rows.append({
                "date": row.get("date", ""),
                "category": row.get("category", ""),
                "description": row.get("description", ""),
                "amount": amt
            })
    return rows

def plot_daily(rows):
    # sum by date (YYYY-MM-DD)
    sums = defaultdict(float)
    for r in rows:
        sums[r["date"]] += r["amount"]
    # sort by date
    items = sorted(
        ((datetime.strptime(d, "%Y-%m-%d"), total) for d, total in sums.items()),
        key=lambda x: x[0]
    )
    if not items:
        print("No data to plot.")
        return
    dates = [d for d, _ in items]
    totals = [t for _, t in items]

    plt.figure()
    plt.plot(dates, totals, marker="o")
    plt.title("Spending by Date")
    plt.xlabel("Date")
    plt.ylabel("Amount")
    plt.tight_layout()
    out = "spending_by_date.png"
    plt.savefig(out, dpi=160)
    print(f"Saved {out}")

def plot_category(rows):
    # sum by category
    sums = defaultdict(float)
    for r in rows:
        cat = r["category"] or "uncat"
        sums[cat] += r["amount"]
    # stable order: highest first
    items = sorted(sums.items(), key=lambda x: x[1], reverse=True)
    if not items:
        print("No data to plot.")
        return
    labels = [k for k, _ in items]
    totals = [v for _, v in items]

    plt.figure()
    plt.bar(labels, totals)
    plt.title("Spending by Category")
    plt.xlabel("Category")
    plt.ylabel("Amount")
    plt.xticks(rotation=20, ha="right")
    plt.tight_layout()
    out = "spending_by_category.png"
    plt.savefig(out, dpi=160)
    print(f"Saved {out}")

def main():
    if len(sys.argv) != 2 or sys.argv[1] not in {"daily", "category"}:
        print(__doc__)
        sys.exit(1)
    rows = read_rows()
    if sys.argv[1] == "daily":
        plot_daily(rows)
    else:
        plot_category(rows)

if __name__ == "__main__":
    main()
