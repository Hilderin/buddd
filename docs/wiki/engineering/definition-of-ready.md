# Definition of Ready

> A feature spec must satisfy all criteria below before it enters implementation.
> Enforced by the **spec-critic** agent. Referenced by the **orchestrator** during the grill-me step.

## Criteria

### Clarity & Completeness

- [ ] Scope is clearly defined (what is included and what is explicitly excluded)
- [ ] Dependencies on other features, modules, or external systems are identified
- [ ] Edge cases and error conditions are described
- [ ] The expected behavior is unambiguous and testable

### Verification

- [ ] The spec defines how the feature will be verified end-to-end
      (demo, screenshot, manual test, script, integration test, etc.)
- [ ] Acceptance criteria are specific, measurable, and verifiable
- [ ] Success and failure states are described

### Documentation

- [ ] Interface changes (CLI flags, API signatures, config keys) are documented
- [ ] Existing documentation that must be updated is listed
      (README, wiki, ADRs, other specs)

### Technical

- [ ] Technical constraints are identified (system APIs, libraries, build changes)
- [ ] Risks or unknowns are surfaced
- [ ] Performance or resource implications, if any, are noted

## Usage

| Agent | When |
|---|---|
| **Orchestrator** | During the grill-me step, walk through each criterion with the human |
| **Spec-author** | Use the criteria as a self-check before declaring the spec complete |
| **Spec-critic** | Check every criterion and report violations as blocking issues |
| **Implementation-contract-critic** | Verify that contract-level decisions still satisfy the original readiness scope |
