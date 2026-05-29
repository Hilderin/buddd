# OpenCode Spec-Driven Development Starter Kit

This starter kit provides a full multi-agent workflow for spec-driven development with OpenCode.

## Agents (`.opencode/agents/`)

| Agent | Role |
|---|---|
| `orchestrator` | Main interface with the human; coordinates the full workflow |
| `repo-scout` | Reads the repository to discover conventions and relevant files |
| `spec-author` | Drafts functional specs from human intent and project context |
| `spec-critic` | Critiques and validates specs for ambiguity, testability, and scope |
| `implementation-contract-author` | Converts accepted specs into precise implementation contracts |
| `implementation-contract-critic` | Critiques and validates implementation contracts |
| `code-implementer` | Implements only accepted implementation contracts |
| `code-reviewer` | Reviews implementation against spec, contract, tests, and constitution |
| `constitution-agent` | Maintains the project constitution when project rules need to change |
| `adr-agent` | Creates ADR proposals for meaningful architectural decisions |
| `wiki-agent` | Maintains the operational project wiki after accepted changes |
| `governance-reviewer` | Performs final cross-document governance validation |

## Skills (`.opencode/skills/`)

- `architecture-governance/`
- `contract-writing/`
- `repo-analysis/`
- `spec-writing/`

## Prompts (`.opencode/prompts/`)

- `governance/`
- `implementation-contract/`
- `orchestrator/`
- `shared/`
- `specification/`

## Core idea

Do not generate code directly from a user request or from a raw spec.

The recommended flow is:

```text
Human intent
  → Orchestrator
  → Proposed spec
  → Spec critique
  → Accepted spec
  → Proposed implementation contract
  → Contract critique
  → Accepted implementation contract
  → Code + tests
  → ADR / constitution / wiki updates
  → Governance review
```

## Authority order

1. `docs/constitution/**`
2. `docs/specs/**`
3. `docs/adr/**`
4. `docs/wiki/**`
5. Existing code conventions

## Documents (`docs/`)

- `adr/` — Architectural Decision Records
- `constitution/` — Mandatory project rules
- `reviews/` — Review reports
- `specs/` — Functional specs (proposed + accepted)
- `templates/` — Templates for specs, contracts, ADRs, reviews, wiki pages
- `wiki/` — Current operational understanding

## Installation

Copy the contents of this folder into the root of your repository.

Then adapt:

- `opencode.json`
- `AGENTS.md`
- `.opencode/agents/*.md`
- `docs/constitution/**`

The files are intentionally conservative. The goal is to reduce randomness during code generation.
