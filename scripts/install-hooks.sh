#!/usr/bin/env bash
# Install Oscil git hooks.
# Usage: ./scripts/install-hooks.sh

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"
HOOKS_DIR="$REPO_ROOT/.git/hooks"

echo "Installing Oscil git hooks..."

# Pre-commit hook
cp "$REPO_ROOT/scripts/pre-commit" "$HOOKS_DIR/pre-commit"
chmod +x "$HOOKS_DIR/pre-commit"
echo "  ✓ pre-commit hook installed"

echo "Done. Hooks are active."
echo ""
echo "To bypass in emergencies: git commit --no-verify"
