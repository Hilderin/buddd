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
