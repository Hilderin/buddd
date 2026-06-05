#!/usr/bin/env bash
# update-references.sh
# Phase 2: Update all references from docs/specs/ to .specs/sprint-YYYY-MM/
#
# Usage: bash scripts/update-references.sh
#
# Handles:
# 1. Wiki pages (docs/wiki/) — maps each spec feature to its sprint folder
# 2. ADRs (docs/adr/) — same mapping
# 3. Root files (AGENTS.md, README.md, SpecKit.md if exists)
# 4. Coordination template (docs/templates/coordination-template.md)
#
# NOTE: Agent prompts (.opencode/agents/*.md) are NOT updated by this script.
# They need a different approach (orchestrator-driven paths).

set -euo pipefail

echo "=== Phase 2: Update references docs/specs/ → .specs/sprint-YYYY-MM/ ==="
echo ""

# Build feature→sprint mapping from .specs/ directory
declare -A FEATURE_SPRINT
for sprint_dir in .specs/sprint-*/; do
  sprint=$(basename "$sprint_dir")
  for feature_dir in "$sprint_dir"*/; do
    feature=$(basename "$feature_dir")
    FEATURE_SPRINT["$feature"]="$sprint"
  done
done

echo "Feature→sprint mapping built (${#FEATURE_SPRINT[@]} entries)."

# Files to update (wiki + ADRs)
FILES_TO_UPDATE=()

# Wiki files (from scout results)
WIKI_FILES=(
  "docs/wiki/architecture/module-map.md"
  "docs/wiki/architecture/overview.md"
  "docs/wiki/architecture/data-flow.md"
  "docs/wiki/domain/glossary.md"
  "docs/wiki/domain/business-rules.md"
  "docs/wiki/decisions/adr-index.md"
  "docs/wiki/architecture/dependency-map.md"
  "docs/wiki/engineering/setup.md"
  "docs/wiki/engineering/testing.md"
  "docs/wiki/engineering/troubleshooting.md"
  "docs/wiki/engineering/deployment.md"
)

# ADR files (from scout results)
ADR_FILES=(
  "docs/adr/ADR-017-multi-material-model.md"
  "docs/adr/013-standard-vertex-format.md"
  "docs/adr/015-ci-docker-image-pre-publishing.md"
  "docs/adr/014-cli-app-system.md"
  "docs/adr/012-navigable-object-graph-engine-service.md"
  "docs/adr/006-rtti-component-dispatch.md"
  "docs/adr/004-demo-system-architecture.md"
  "docs/adr/003-render-pipeline-architecture.md"
  "docs/adr/002-glm-wrapper-math.md"
)

# Other root files
ROOT_FILES=(
  "AGENTS.md"
  "SpecKit.md"
  "README.md"
)

TEMPLATE_FILES=(
  "docs/templates/coordination-template.md"
)

echo "=== 1. Updating wiki files ==="
UPDATED=0
for file in "${WIKI_FILES[@]}"; do
  if [ ! -f "$file" ]; then
    echo "  SKIP $file (not found)"
    continue
  fi
  changes=0
  for feature in "${!FEATURE_SPRINT[@]}"; do
    sprint="${FEATURE_SPRINT[$feature]}"
    old_path="docs/specs/$feature"
    new_path=".specs/$sprint/$feature"
    # Count matches before replacement
    if grep -q "$old_path" "$file" 2>/dev/null; then
      # Use sed to replace all occurrences
      sed -i "s|$old_path|$new_path|g" "$file"
      changes=$((changes + 1))
    fi
  done
  if [ "$changes" -gt 0 ]; then
    echo "  ✅ $file ($changes references updated)"
    UPDATED=$((UPDATED + 1))
  else
    echo "  - $file (no changes)"
  fi
done

echo ""
echo "=== 2. Updating ADR files ==="
for file in "${ADR_FILES[@]}"; do
  if [ ! -f "$file" ]; then
    echo "  SKIP $file (not found)"
    continue
  fi
  changes=0
  for feature in "${!FEATURE_SPRINT[@]}"; do
    sprint="${FEATURE_SPRINT[$feature]}"
    old_path="docs/specs/$feature"
    new_path=".specs/$sprint/$feature"
    if grep -q "$old_path" "$file" 2>/dev/null; then
      sed -i "s|$old_path|$new_path|g" "$file"
      changes=$((changes + 1))
    fi
  done
  if [ "$changes" -gt 0 ]; then
    echo "  ✅ $file ($changes references updated)"
    UPDATED=$((UPDATED + 1))
  else
    echo "  - $file (no changes)"
  fi
done

echo ""
echo "=== 3. Updating root files ==="
for file in "${ROOT_FILES[@]}"; do
  if [ ! -f "$file" ]; then
    echo "  SKIP $file (not found)"
    continue
  fi
  changes=0
  for feature in "${!FEATURE_SPRINT[@]}"; do
    sprint="${FEATURE_SPRINT[$feature]}"
    old_path="docs/specs/$feature"
    new_path=".specs/$sprint/$feature"
    if grep -q "$old_path" "$file" 2>/dev/null; then
      sed -i "s|$old_path|$new_path|g" "$file"
      changes=$((changes + 1))
    fi
  done
  # Also replace generic docs/specs/ references (without specific feature)
  if grep -q "docs/specs/" "$file" 2>/dev/null; then
    # Only replace generic patterns that aren't feature-specific
    # This should handle cases like "docs/specs/**" or "docs/specs/"
    sed -i "s|docs/specs/|.specs/|g" "$file"
    changes=$((changes + 1))
  fi
  if [ "$changes" -gt 0 ]; then
    echo "  ✅ $file ($changes updates)"
    UPDATED=$((UPDATED + 1))
  else
    echo "  - $file (no changes)"
  fi
done

echo ""
echo "=== 4. Updating template files ==="
for file in "${TEMPLATE_FILES[@]}"; do
  if [ ! -f "$file" ]; then
    echo "  SKIP $file (not found)"
    continue
  fi
  changes=0
  # Replace feature-specific paths first
  for feature in "${!FEATURE_SPRINT[@]}"; do
    sprint="${FEATURE_SPRINT[$feature]}"
    old_path="docs/specs/$feature"
    new_path=".specs/$sprint/$feature"
    if grep -q "$old_path" "$file" 2>/dev/null; then
      sed -i "s|$old_path|$new_path|g" "$file"
      changes=$((changes + 1))
    fi
  done
  # Replace generic docs/specs/<feature>/ pattern (used in templates)
  if grep -q "docs/specs/<feature>" "$file" 2>/dev/null; then
    sed -i "s|docs/specs/<feature>|.specs/{{SPRINT}}/<feature>|g" "$file"
    changes=$((changes + 1))
  fi
  # Replace bare docs/specs/ references
  if grep -q "docs/specs/" "$file" 2>/dev/null; then
    sed -i "s|docs/specs/|.specs/|g" "$file"
    changes=$((changes + 1))
  fi
  if [ "$changes" -gt 0 ]; then
    echo "  ✅ $file ($changes updates)"
    UPDATED=$((UPDATED + 1))
  else
    echo "  - $file (no changes)"
  fi
done

echo ""
echo "=== Summary ==="
echo "  Files updated: $UPDATED"
echo ""
echo "Next step: Update agent prompts in .opencode/agents/*.md"
echo "Then update AGENTS.md authority order."
