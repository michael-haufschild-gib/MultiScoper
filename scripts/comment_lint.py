#!/usr/bin/env python3
"""
Comment quality lint for C/C++ source files.

Enforces:
1. No empty comments (// with nothing after, /* */ with only whitespace)
2. Public API functions in include/ headers must have a doc comment

Exit code 1 on any violation.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator, List, Optional, Sequence

EXCLUDED_DIR_NAMES = {"build", ".git", ".serena", ".claude", "_deps"}
DEFAULT_EXTENSIONS = {".h", ".hh", ".hpp", ".hxx"}
ALL_CODE_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}

# Matches empty single-line comments: // followed by optional whitespace only
EMPTY_LINE_COMMENT_RE = re.compile(r"^\s*//\s*$")

# Matches empty block comments on a single line: /* */ or /**/
EMPTY_BLOCK_COMMENT_RE = re.compile(r"/\*\s*\*/")

# Matches a function declaration/definition signature (simplified heuristic).
# Captures: optional return type + name + ( params ) + qualifiers + { or ;
FUNCTION_DECL_RE = re.compile(
    r"^\s*"
    r"(?!(?:if|for|while|switch|catch|return|else|do|try|class|struct"
    r"|namespace|enum|union|using|typedef|template|static_assert"
    r"|JUCE_DECLARE|JUCE_LEAK|jassert|DBG)\b)"
    r"[a-zA-Z_][\w:*&<>, ]*\s+"  # return type (at least one space before name)
    r"[a-zA-Z_]\w*"               # function name
    r"\s*\([^;]*\)"               # parameter list
    r"[^;{]*[;{]"                 # trailing qualifiers + ; or {
)

# Function declaration: return_type name(params) qualifiers;
# Must have: return type (or void), function name, parens, semicolon.
# Must NOT have: = (assignment), . (member access), -> (pointer access), [ (array).
SIMPLE_DECL_RE = re.compile(
    r"^\s*"
    # Negative lookahead for non-declaration keywords
    r"(?!(?:if|for|while|switch|catch|return|else|do|try|class|struct"
    r"|namespace|enum|union|using|typedef|static_assert|static_cast"
    r"|dynamic_cast|reinterpret_cast|const_cast|auto "
    r"|JUCE_DECLARE|JUCE_LEAK|jassert|DBG|#)\b)"
    # Optional attributes like [[nodiscard]]
    r"(?:\[\[[\w:]+\]\]\s*)?"
    # Return type: identifier(s) with optional ::, *, &, <>, and spaces
    r"[\w][\w:*&<>, ]*\s+"
    # Function name: identifier (not starting with a digit)
    r"[a-zA-Z_~]\w*"
    # Parameter list
    r"\s*\([^)]*\)\s*"
    # Optional trailing qualifiers
    r"(?:const|override|final|noexcept|volatile|&|&&|\s)*"
    r";\s*$"
)

# Constructor/destructor pattern: ClassName( or ~ClassName(
# Must look like a constructor: PascalCase name, not ALL_CAPS or mixed (Win32 API calls)
CTOR_DTOR_RE = re.compile(
    r"^\s*(?:explicit\s+)?~?[A-Z][a-z]\w*\s*\("
)

# Lines that are definitely not function declarations
NON_FUNCTION_PREFIXES = (
    "#", "//", "/*", "*", "}", "friend ", "template",
    "public:", "private:", "protected:",
)

# Declarations that don't need doc comments — boilerplate with zero ambiguity
EXEMPT_PATTERNS = [
    # Deleted/defaulted special members: = delete / = default
    re.compile(r"=\s*(?:delete|default)\s*;\s*$"),
    # Destructors: ~ClassName()
    re.compile(r"^\s*(?:virtual\s+)?~\w+\s*\("),
    # Trivial getters: getX() const, isX() const, hasX() const (no params)
    re.compile(
        r"(?:get|is|has|was|can|should)[A-Z]\w*\s*\(\s*\)\s*const"
    ),
    # Trivial setters: setX(single_param) — name is self-documenting
    re.compile(
        r"void\s+set[A-Z]\w*\s*\([^,)]+\)\s*;"
    ),
    # Listener add/remove: addListener(T*), removeListener(T*)
    re.compile(
        r"void\s+(?:add|remove)Listener\s*\("
    ),
    # JUCE overrides — documented by the framework, not by us
    re.compile(
        r"\b(?:paint|resized|mouseDown|mouseUp|mouseMove|mouseEnter|mouseExit"
        r"|mouseDoubleClick|mouseDrag|mouseWheelMove"
        r"|keyPressed|focusGained|focusLost|parentHierarchyChanged"
        r"|prepareToPlay|releaseResources|processBlock|isBusesLayoutSupported"
        r"|createEditor|getStateInformation|setStateInformation"
        r"|getName|acceptsMidi|producesMidi|isMidiEffect|getTailLengthSeconds"
        r"|getNumPrograms|getCurrentProgram|setCurrentProgram|getProgramName"
        r"|changeProgramName|hasEditor"
        r"|newOpenGLContextCreated|renderOpenGL|openGLContextClosing"
        r"|timerCallback|handleAsyncUpdate|valueTreePropertyChanged"
        r"|valueTreeChildAdded|valueTreeChildRemoved|buttonClicked"
        r"|sliderValueChanged|comboBoxChanged|textEditorTextChanged"
        r"|textEditorReturnKeyPressed|textEditorEscapeKeyPressed"
        r"|textEditorFocusLost|visibilityChanged|lookAndFeelChanged"
        r"|colourChanged|enablementChanged|moved|childBoundsChanged"
        r"|broughtToFront|getTooltip|createAccessibilityHandler"
        r"|hitTest|themeChanged)\s*\("
    ),
    # Project listener overrides — documented on the listener interface
    re.compile(
        r"void\s+(?:source(?:Added|Removed|Updated)"
        r"|oscillator(?:Selected|VisibilityChanged|ModeChanged|ConfigRequested"
        r"|ColorConfigRequested|DeleteRequested|DragStarted|MoveRequested"
        r"|PaneSelectionRequested|NameChanged|sReordered)"
        r"|filterModeChanged"
        r"|column(?:LayoutChanged)|pane(?:OrderChanged|Added|Removed)"
        r"|addOscillator(?:DialogRequested|Requested)"
        r"|itemDrag(?:Enter|Move|Exit)|itemDropped"
        r"|isInterestedInDragSource"
        r"|valueTreeChildOrderChanged|valueTreeParentChanged)\s*\("
    ),
    # Operator overloads
    re.compile(r"\boperator\s*[^\w\s(]"),
    # Inline trivial bodies on same line: { return ...; }
    re.compile(r"\{[^{}]*return[^{}]*;\s*\}\s*$"),
    # JUCE macros that generate declarations
    re.compile(r"^\s*JUCE_DECLARE_"),
    # Variable declarations / function calls with value args (strings, bare numbers)
    re.compile(r'\w+\s*\([^)]*"'),
    re.compile(r"\w+\s*\(\s*\d"),
    # Function calls with ALL_CAPS macro arguments (not parameter declarations)
    re.compile(r"\w+\s*\(\s*[A-Z_]{3,}"),
    # Static variable declarations (not static member functions — those have void/type before name)
    re.compile(r"^\s*static\s+(?![\w:]+\s+\w+\s*\()"),
    # const RAII local variable declarations: const Type varName(memberArg_);
    re.compile(r"^\s*const\s+[\w:]+\s+\w+\s*\(\w+_\)"),
    # Local variable construction: Type name(arg); — looks like a declaration but isn't
    re.compile(r"^\s*[\w:]+\s+\w+\s*\(\w+\)\s*;\s*$"),
    # UI component constructors: ClassName(IThemeService& ...) — boilerplate DI wiring
    re.compile(r"^\s*(?:explicit\s+)?\w+\s*\(\s*IThemeService\s*&"),
    # Widget value setters with notify flag: setValue(T, bool notify = true)
    re.compile(
        r"void\s+(?:set(?:Selected(?:Index|Id|Indices)?|Value|RangeValues|Expanded"
        r"|TabBadge|TabEnabled|NumericValue))\s*\("
    ),
    # Widget group management: addOption(s), addTab(s), clearOptions/Tabs
    re.compile(
        r"void\s+(?:add(?:Option|Options|Tab|Tabs|MagneticPoint|Oscillator)"
        r"|clear(?:Options|Tabs|MagneticPoints|Sections|Error|Icon|Oscillators)"
        r"|remove(?:Section|Oscillator)|detachFromParameter"
        r"|setSelectedById)\s*\("
    ),
    # Oscillator management methods — domain-specific but self-documenting
    re.compile(
        r"void\s+(?:updateOscillator(?:Source|Name|Color|Full)?|highlightOscillator"
        r"|requestWaveformRestartAtTimestamp)\s*\("
    ),
    # Common self-documenting methods
    re.compile(r"void\s+(?:toggle|update|showPopup|hidePopup)\s*\(\s*\)\s*;"),
    # setRange(min, max) — self-documenting
    re.compile(r"void\s+setRange\s*\("),
    # Default constructors — ClassName();
    re.compile(r"^\s*\w+\s*\(\s*\)\s*;\s*$"),
    # HTTP handler methods — handle{Action}(req, res) pattern is self-documenting
    re.compile(r"void\s+handle\w+\s*\(.*httplib::(Request|Response)"),
    # Test server lifecycle methods
    re.compile(r"void\s+(?:start|stop)\s*\("),
]

# Doc comment: line immediately above is /// or /** or ends with */
DOC_COMMENT_RE = re.compile(r"^\s*(?:///|/\*\*|//!)")
BLOCK_COMMENT_END_RE = re.compile(r"\*/\s*$")


@dataclass(frozen=True)
class EmptyCommentViolation:
    path: str
    line_number: int
    line_text: str


@dataclass(frozen=True)
class MissingDocViolation:
    path: str
    line_number: int
    signature: str


def iter_files(
    root: Path,
    scan_paths: Sequence[str],
    extensions: set[str],
) -> Iterator[Path]:
    ext_set = {ext.lower() for ext in extensions}
    for relative in scan_paths:
        start = (root / relative).resolve()
        if not start.exists():
            continue
        for path in start.rglob("*"):
            if not path.is_file():
                continue
            if path.suffix.lower() not in ext_set:
                continue
            if any(part in EXCLUDED_DIR_NAMES for part in path.parts):
                continue
            yield path


def find_empty_comments(
    path: Path, root: Path
) -> List[EmptyCommentViolation]:
    violations: List[EmptyCommentViolation] = []
    relative = str(path.relative_to(root))
    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()

    in_block_comment = False
    block_start: Optional[int] = None
    block_content_seen = False

    for line_no, line in enumerate(lines, start=1):
        # Track block comment state
        if in_block_comment:
            # Check for content (non-whitespace, non-* decoration)
            stripped = line.strip().lstrip("*").strip()
            if stripped and stripped != "/":
                block_content_seen = True
            if "*/" in line:
                in_block_comment = False
                if not block_content_seen and block_start is not None:
                    violations.append(
                        EmptyCommentViolation(relative, block_start, "/* */")
                    )
            continue

        # Single-line empty block comment: /* */ or /**/
        if EMPTY_BLOCK_COMMENT_RE.search(line):
            # Check the content between /* and */
            match = re.search(r"/\*(.*?)\*/", line)
            if match and not match.group(1).strip():
                violations.append(
                    EmptyCommentViolation(relative, line_no, line.rstrip())
                )

        # Empty single-line comment: // with nothing after
        # Allow blank // as paragraph separators between other // comment lines
        if EMPTY_LINE_COMMENT_RE.match(line):
            prev_is_comment = (line_no >= 2 and
                               lines[line_no - 2].strip().startswith("//"))
            next_is_comment = (line_no < len(lines) and
                               lines[line_no].strip().startswith("//"))
            if not (prev_is_comment and next_is_comment):
                violations.append(
                    EmptyCommentViolation(relative, line_no, line.rstrip())
                )

        # Start of block comment
        if "/*" in line and "*/" not in line:
            in_block_comment = True
            block_start = line_no
            block_content_seen = False
            # Check if opening line has content after /*
            after_open = line[line.index("/*") + 2 :].strip().lstrip("*").strip()
            if after_open:
                block_content_seen = True

    return violations


def is_exempt(line: str) -> bool:
    """Return True if this declaration is boilerplate that doesn't need a doc comment."""
    return any(pat.search(line) for pat in EXEMPT_PATTERNS)


def is_function_declaration(line: str) -> bool:
    """Heuristic: does this line look like a public function declaration that needs a doc?"""
    stripped = line.strip()

    # Skip lines that can't be function declarations
    if not stripped or any(stripped.startswith(p) for p in NON_FUNCTION_PREFIXES):
        return False

    # Must contain parentheses
    if "(" not in stripped:
        return False

    # Skip boilerplate that doesn't benefit from doc comments
    if is_exempt(stripped):
        return False

    # Constructor/destructor
    if CTOR_DTOR_RE.match(stripped) and stripped.rstrip().endswith(";"):
        return True

    # Standard function declaration ending with ;
    if SIMPLE_DECL_RE.match(stripped):
        return True

    return False


def has_doc_comment(lines: List[str], decl_line_idx: int) -> bool:
    """Check if the line above the declaration (or nearby) has a doc comment."""
    # Walk upward past blank lines and find the nearest comment
    idx = decl_line_idx - 1
    while idx >= 0:
        stripped = lines[idx].strip()
        if not stripped:
            idx -= 1
            continue
        # Found a non-blank line — is it a doc comment or end of block comment?
        if DOC_COMMENT_RE.match(lines[idx]):
            return True
        if BLOCK_COMMENT_END_RE.search(lines[idx]):
            return True
        # It's something else (code, non-doc comment) — no doc
        return False
    return False


def find_missing_docs(
    path: Path, root: Path
) -> List[MissingDocViolation]:
    """Check public header files for undocumented function declarations."""
    violations: List[MissingDocViolation] = []
    relative = str(path.relative_to(root))
    content = path.read_text(encoding="utf-8", errors="ignore")
    lines = content.splitlines()

    in_block_comment = False
    access_level = "public"  # Default for non-class context in headers

    for line_no_1based, line in enumerate(lines, start=1):
        idx = line_no_1based - 1
        stripped = line.strip()

        # Track block comments
        if in_block_comment:
            if "*/" in line:
                in_block_comment = False
            continue
        if "/*" in stripped and "*/" not in stripped:
            in_block_comment = True
            continue

        # Skip single-line comments
        if stripped.startswith("//"):
            continue

        # Track access specifiers
        if stripped.startswith("public:"):
            access_level = "public"
            continue
        if stripped.startswith("private:") or stripped.startswith("protected:"):
            access_level = "private"
            continue

        # Only lint public declarations
        if access_level != "public":
            continue

        # Only lint declarations (ending with ;), not definitions (ending with {)
        # This naturally excludes function bodies since body lines don't end with ;
        # in the declaration pattern we're matching.
        if is_function_declaration(stripped):
            if not has_doc_comment(lines, idx):
                sig = stripped[:100] + ("..." if len(stripped) > 100 else "")
                violations.append(
                    MissingDocViolation(relative, line_no_1based, sig)
                )

    return violations


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Enforce comment quality: no empty comments, "
        "public API functions must have doc comments."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="Repository root path.",
    )
    parser.add_argument(
        "--paths",
        nargs="+",
        default=["include", "src", "tests", "test_harness"],
        help="Top-level paths to scan for empty comments.",
    )
    parser.add_argument(
        "--doc-paths",
        nargs="+",
        default=["include"],
        help="Top-level paths to scan for missing doc comments (headers only).",
    )
    parser.add_argument(
        "--extensions",
        nargs="+",
        default=sorted(ALL_CODE_EXTENSIONS),
        help="File extensions for empty comment check.",
    )
    parser.add_argument(
        "--doc-extensions",
        nargs="+",
        default=sorted(DEFAULT_EXTENSIONS),
        help="File extensions for doc comment check.",
    )
    parser.add_argument(
        "--skip-empty-comments",
        action="store_true",
        help="Skip the empty comment check.",
    )
    parser.add_argument(
        "--skip-doc-comments",
        action="store_true",
        help="Skip the missing doc comment check.",
    )
    parser.add_argument(
        "--max-violations",
        type=int,
        default=0,
        help="Maximum allowed violations (ratchet). 0 = no violations allowed.",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    root = args.root.resolve()

    empty_violations: List[EmptyCommentViolation] = []
    doc_violations: List[MissingDocViolation] = []
    scanned_empty = 0
    scanned_doc = 0

    if not args.skip_empty_comments:
        for path in iter_files(root, args.paths, set(args.extensions)):
            scanned_empty += 1
            empty_violations.extend(find_empty_comments(path, root))

    if not args.skip_doc_comments:
        for path in iter_files(root, args.doc_paths, set(args.doc_extensions)):
            scanned_doc += 1
            doc_violations.extend(find_missing_docs(path, root))

    empty_violations.sort(key=lambda v: (v.path, v.line_number))
    doc_violations.sort(key=lambda v: (v.path, v.line_number))

    print("Comment lint configuration:")
    print(f"  root: {root}")
    if not args.skip_empty_comments:
        print(f"  empty comment paths: {', '.join(args.paths)}")
        print(f"  empty comment files scanned: {scanned_empty}")
    if not args.skip_doc_comments:
        print(f"  doc comment paths: {', '.join(args.doc_paths)}")
        print(f"  doc comment files scanned: {scanned_doc}")

    if empty_violations:
        print(f"\nEmpty comment violations ({len(empty_violations)}):")
        for v in empty_violations:
            print(f"  {v.path}:{v.line_number}: {v.line_text}")

    if doc_violations:
        print(f"\nMissing doc comment violations ({len(doc_violations)}):")
        for v in doc_violations:
            print(f"  {v.path}:{v.line_number}: {v.signature}")

    total = len(empty_violations) + len(doc_violations)
    max_allowed = args.max_violations

    if total > max_allowed:
        print(f"\nFAILED: {total} comment violation(s) (max allowed: {max_allowed}).")
        return 1

    if total > 0:
        print(f"\nPASSED (ratchet): {total} violation(s) within allowed threshold of {max_allowed}.")
    else:
        print("\nPASSED: no comment violations found.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
