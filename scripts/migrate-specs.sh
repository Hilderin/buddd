#!/usr/bin/env bash
# migrate-specs.sh
# Phase 1: Migrate specs from docs/specs/<feature>/ to .specs/sprint-YYYY-MM/<feature>/
#
# Usage: bash scripts/migrate-specs.sh
#
# This script:
# 1. Determines the sprint (YYYY-MM) for each spec from git creation date
# 2. Copies files to .specs/sprint-YYYY-MM/<feature>/
# 3. Leaves originals in place for verification
# 4. Outputs a summary of what was done

set -euo pipefail

MIGRATION_LOG=$(mktemp)
SPECS_ROOT="docs/specs"
NEW_ROOT=".specs"

echo "=== Spec Migration: docs/specs/ → .specs/sprint-YYYY-MM/ ==="
echo ""

# Check we're in the repo root
if [ ! -d "$SPECS_ROOT" ]; then
  echo "ERROR: $SPECS_ROOT not found. Run from repo root."
  exit 1
fi

mkdir -p "$NEW_ROOT"

# Create .specs README
cat > "$NEW_ROOT/README.md" << 'README'
# Specs

Historical snapshots of feature intent, organized by sprint.
See [docs/wiki/](../docs/wiki/) for current operational understanding.
See [docs/adr/](../docs/adr/) for architectural decisions.

## Structure

```
.specs/
  README.md
  sprint-YYYY-MM/
    <feature>/
      spec.md                    # Functional spec (historical)
      coordination.md            # Workflow decisions and process log
      implementation-contract.md # What was actually implemented
```

Process artifacts (critic reviews, code reviews, governance validation) are not
retained in the archive — they exist only during the active workflow.

## Usage

- **Active workflow**: The orchestrator creates specs in the current sprint folder.
- **Historical reference**: Older sprints are read-only. Do not modify archived specs.
- **Authority order**: Specs are authority #4 (below wiki). The wiki is the source
  of current truth.
README

# Find all feature directories
FEATURES=()
for dir in "$SPECS_ROOT"/*/; do
  FEATURES+=("$(basename "$dir")")
done

echo "Found ${#FEATURES[@]} feature directories to migrate."
echo ""

MIGRATED=0
SKIPPED=0

for feature in "${FEATURES[@]}"; do
  src="$SPECS_ROOT/$feature"

  # Determine sprint from git creation date of the directory
  sprint=$(git log --diff-filter=A --follow --format="%ad" --date=format:"%Y-%m" -- "$src" 2>/dev/null | tail -1)

  if [ -z "$sprint" ]; then
    # Fallback: earliest commit touching this directory
    sprint=$(git log --reverse --format="%ad" --date=format:"%Y-%m" -- "$src" 2>/dev/null | head -1)
  fi

  if [ -z "$sprint" ]; then
    echo "  ⚠ SKIP: $feature (no git date found)"
    SKIPPED=$((SKIPPED + 1))
    continue
  fi

  dst="$NEW_ROOT/sprint-$sprint/$feature"

  # Check if destination already exists
  if [ -d "$dst" ]; then
    echo "  ⚠ SKIP: $feature → sprint-$sprint/ (destination already exists at $dst)"
    SKIPPED=$((SKIPPED + 1))
    continue
  fi

  # Copy
  mkdir -p "$dst"
  cp -r "$src"/* "$dst/"

  echo "  ✅ $feature → sprint-$sprint/"
  MIGRATED=$((MIGRATED + 1))
done

echo ""
echo "=== Summary ==="
echo "  Migrated: $MIGRATED"
echo "  Skipped:  $SKIPPED"
echo ""
echo "Originals preserved at $SPECS_ROOT/"
echo "Verify the results, then run: rm -rf $SPECS_ROOT"
echo ""
echo "Next step: Update all references from docs/specs/ to .specs/"
