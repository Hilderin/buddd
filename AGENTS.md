# Agent Operating Rules

These rules apply to every agent working in this repository.

## Authority order

1. `docs/constitution/**`
2. `docs/specs/**`
3. `docs/adr/**`
4. `docs/wiki/**`
5. Existing code conventions

## Non-negotiable rules

- Do not implement code directly from a raw user request.
- Do not implement code directly from a spec.
- Code may only be implemented from an accepted implementation contract.
- If the implementation contract is ambiguous, stop and escalate to the Orchestrator.
- Do not violate constitution rules.
- Do not silently change architecture.
- Do not directly modify accepted constitution rules.
- Do not rewrite accepted ADR history.
- Do not update the wiki in a way that contradicts the constitution or accepted ADRs.

## Document roles

- Constitution: mandatory project rules.
- ADRs: historical decisions and rationale.
- Wiki: current operational understanding.
- Specs: product and behavior intent.
- Implementation contracts: constrained implementation instructions.

## Escalation

Escalate to the Orchestrator when:

- The requested work conflicts with the constitution.
- The spec is ambiguous or not testable.
- The implementation contract does not constrain the work enough.
- A new architectural decision is required.
- A dependency, framework, service, or persistence strategy must change.

## Wiki tools

All agents have access to wiki search tools that query the operational wiki at `docs/wiki/**`:
- `wiki_wiki_search` — hybrid full-text and semantic search
- `wiki_wiki_search_exact` — exact lexical/FTS search
- `wiki_wiki_read_section` — read a specific section by path and heading
- `wiki_wiki_status` — check index status
- `wiki_wiki_reindex` — force reindex

Before making decisions, writing documents, reviewing work, or implementing code, agents should proactively search the wiki for relevant operational context. The wiki captures current understanding of architecture, domain concepts, engineering practices, and decisions.

The wiki sits at authority order #4 — above existing code conventions — and should be consulted as a primary reference for operational knowledge.
