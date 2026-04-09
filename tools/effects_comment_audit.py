#!/usr/bin/env python3
"""
Heuristic audit: which effects/*/*.c files look under-documented for tutorial-style
headers (long first comment, demoscene/hardware keywords).

Usage (from repo root):
  python3 tools/effects_comment_audit.py
  python3 tools/effects_comment_audit.py --json

Does NOT prove comments are good — only flags files worth human review.
"""
from __future__ import annotations

import argparse
import json
import os
import re

EFFECTS = "effects"
KEYWORDS = (
    "demoscene",
    "technique",
    "purpose",
    "copper",
    "blitter",
    "bplcon",
    "dma",
    "why ",
    "hrm",
    "conceptual",
    "classic ",
    "oamiga",
)


def header_comments(text: str) -> tuple[str, int]:
    """
    Concatenate consecutive /* */ blocks at file start (before first #include),
    so files with a short stub + a second real paragraph score correctly.
    """
    t = text.lstrip()
    if not t.startswith("/*"):
        return "", 0
    pos = 0
    chunks: list[str] = []
    line_total = 0
    while True:
        if not t[pos:].lstrip().startswith("/*"):
            break
        pos = t.find("/*", pos)
        if pos < 0:
            break
        end = t.find("*/", pos)
        if end < 0:
            chunks.append(t[pos:])
            line_total += t[pos:].count("\n")
            break
        chunk = t[pos : end + 2]
        chunks.append(chunk)
        line_total += chunk.count("\n")
        pos = end + 2
        rest = t[pos:].lstrip()
        if rest.startswith("#include") or rest.startswith("#ifndef"):
            break
        if not rest.startswith("/*"):
            break
    block = "".join(chunks)
    return block, line_total


def score_file(path: str) -> dict:
    with open(path, encoding="utf-8", errors="replace") as f:
        text = f.read()
    block, blines = header_comments(text)
    low = block.lower()
    kw_hits = sum(1 for k in KEYWORDS if k in low)
    # Short generic stub many files still have
    stub = "visual effect module" in low and blines < 20
    # Strong signal: substantive header
    strong = blines >= 28 or kw_hits >= 4
    return {
        "path": path.replace("\\", "/"),
        "first_comment_lines": blines,
        "keyword_hits": kw_hits,
        "likely_stub_header": stub,
        "likely_substantial": strong,
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--json", action="store_true", help="print JSON lines")
    args = ap.parse_args()

    rows: list[dict] = []
    for dirpath, dirnames, filenames in os.walk(EFFECTS):
        if "data" in dirpath.split(os.sep):
            continue
        for fn in filenames:
            if not fn.endswith(".c"):
                continue
            path = os.path.join(dirpath, fn)
            rows.append(score_file(path))

    # Priority: review stubs and low keyword hits first
    def sort_key(r: dict):
        return (
            r["likely_substantial"],
            r["keyword_hits"],
            r["first_comment_lines"],
            r["path"],
        )

    rows.sort(key=sort_key)

    need_review = [r for r in rows if not r["likely_substantial"] or r["likely_stub_header"]]

    if args.json:
        for r in rows:
            print(json.dumps(r, ensure_ascii=False))
        return

    print("effects_comment_audit.py — heuristic only\n")
    print(f"Total effect sources (excl. */data/*): {len(rows)}")
    print(f"Flagged for review (not 'substantial' OR stub header): {len(need_review)}\n")
    print("--- Lowest quality first (keyword_hits, first_comment_lines, path) ---")
    for r in rows:
        if r["likely_substantial"]:
            continue
        print(
            f"  kw={r['keyword_hits']:2d}  lines={r['first_comment_lines']:3d}  "
            f"stub={r['likely_stub_header']}  {r['path']}"
        )
    print("\n--- Files marked substantial (quick pass OK) ---")
    for r in rows:
        if r["likely_substantial"]:
            print(f"  {r['path']}")


if __name__ == "__main__":
    main()
