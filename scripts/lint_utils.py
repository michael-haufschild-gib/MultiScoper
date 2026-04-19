#!/usr/bin/env python3
"""Shared helpers for MultiScoper lint scripts.

The two Python linters in scripts/ (forbidden_patterns_lint.py and
test_quality_lint.py) both need to sanitize C/C++ source by removing
comments and string-literal contents before regex-scanning. A drift
between the two state machines produced subtly different edge cases and
one regression (wrapped raw string literals containing rationale text)
that took hours to track down. The canonical implementation lives here
so any future parser fix lands in both callers simultaneously.
"""

from __future__ import annotations

import re

_RAW_STRING_OPEN = re.compile(r'(?:u8|u|U|L)?R"([^(\\\s]*)\(')
_STRING_PREFIX = re.compile(r'(?:u8|u|U|L)')


def _is_identifier_char(ch: str) -> bool:
    return ch.isalnum() or ch == "_"


def strip_comments_and_strings(content: str) -> str:
    """Blank C/C++ comments and string/char literal contents with spaces
    while preserving line offsets (newlines are retained as-is).

    Handles:
      * ``// line comments`` up to the next newline.
      * ``/* block comments */`` — including ones spanning newlines.
      * Regular ``"..."`` and ``'...'`` literals with ``\\`` escapes.
      * Encoding prefixes (``L``, ``u``, ``u8``, ``U``) on both regular
        and raw literals.
      * Raw string literals (``R"delim(...)delim"``) including
        prefixed variants (``LR"..."``, ``u8R"..."`` etc.).

    A character-wise state machine (rather than layered regexes) is
    used because regex approaches are order-sensitive and have
    repeatedly misidentified comments inside string literals and
    vice versa.
    """
    out: list[str] = []
    i = 0
    n = len(content)
    while i < n:
        c = content[i]
        c2 = content[i : i + 2]

        # Block comment.
        if c2 == "/*":
            end = content.find("*/", i + 2)
            if end == -1:
                end = n
            else:
                end += 2
            out.append(re.sub(r"[^\n]", " ", content[i:end]))
            i = end
            continue

        # Line comment.
        if c2 == "//":
            end = content.find("\n", i)
            if end == -1:
                end = n
            out.append(" " * (end - i))
            i = end
            continue

        prev_is_ident = i > 0 and _is_identifier_char(content[i - 1])

        # Raw string literal (optionally prefixed).
        if not prev_is_ident:
            raw_match = _RAW_STRING_OPEN.match(content, i)
            if raw_match:
                delim = raw_match.group(1)
                close = f"){delim}\""
                end = content.find(close, raw_match.end())
                if end == -1:
                    end = n
                else:
                    end += len(close)
                out.append(re.sub(r"[^\n]", " ", content[i:end]))
                i = end
                continue

        # Prefixed non-raw literal: consume the prefix then let the
        # regular string handler below take over.
        if not prev_is_ident and c in ("u", "U", "L"):
            prefix_match = _STRING_PREFIX.match(content, i)
            if (
                prefix_match
                and prefix_match.end() < n
                and content[prefix_match.end()] in ('"', "'")
            ):
                out.append(" " * (prefix_match.end() - i))
                i = prefix_match.end()
                c = content[i]

        # Regular string or char literal.
        if c in ('"', "'"):
            quote = c
            start = i
            i += 1
            while i < n:
                ch = content[i]
                if ch == "\\" and i + 1 < n:
                    i += 2
                    continue
                if ch == quote or ch == "\n":
                    if ch == quote:
                        i += 1
                    break
                i += 1
            out.append(re.sub(r"[^\n]", " ", content[start:i]))
            continue

        out.append(c)
        i += 1

    return "".join(out)
