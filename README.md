# OpenCode Spec-Driven Development Starter Kit

This starter kit provides a multi-agent folder structure for a spec-driven development workflow with:

- A main Orchestrator Agent
- Spec authoring and spec review agents
- Implementation contract authoring and contract review agents
- Code and test execution agents
- Constitution, ADR, and Wiki governance agents
- Templates for specs, implementation contracts, ADRs, constitution rules, amendments, and reviews

## Core idea

Do not generate code directly from a user request or from a raw spec.

The recommended flow is:

```text
Human intent
  -> Orchestrator
  -> Proposed spec
  -> Spec critique
  -> Accepted spec
  -> Proposed implementation contract
  -> Contract critique
  -> Accepted implementation contract
  -> Code + tests
  -> ADR / constitution / wiki updates
  -> Governance review
```

## Authority order

1. Constitution
2. Accepted specs
3. Accepted implementation contracts
4. Accepted ADRs
5. Wiki
6. Existing code conventions

## Installation

Copy the contents of this folder into the root of your repository.

Then adapt:

- `opencode.json`
- `AGENTS.md`
- `.opencode/agents/*.md`
- `docs/constitution/**`

The files are intentionally conservative. The goal is to reduce randomness during code generation.
