# Spec-Driven Development Experiments

> A log of my experiments, observations, and findings while iterating on a spec-driven development workflow using LLMs.
>
> **Primary model used throughout:** DeepSeek V4 Flash (OpenCode).

---

## Table of Contents

1. [Iteration #1: Big Upfront Documentation + SpecKit](#iteration-1-big-upfront-documentation--speckit)
2. [Iteration #2: Iterative Development with Governance Agents](#iteration-2-iterative-development-with-governance-agents)
3. [Iteration #3: Adding a Wiki for Long-Term Memory](#iteration-3-adding-a-wiki-for-long-term-memory)
4. [Iteration #4: Vision MCP for Visual Validation](#iteration-4-vision-mcp-for-visual-validation)
5. [Iteration #5: Coordination File to Reduce Orchestrator Context](#iteration-5-coordination-file-to-reduce-orchestrator-context)
6. [Iteration #6: Grill-Me Step for Deeper Spec Clarification](#iteration-6-grill-me-step-for-deeper-spec-clarification)
7. [Iteration #7: Definition of Ready](#iteration-7-definition-of-ready)
8. [Iteration #8: File Update Protocol — Partial Edits Instead of Full Rewrites](#iteration-8-file-update-protocol--partial-edits-instead-of-full-rewrites)
9. [Cross-Cutting Observations](#cross-cutting-observations)
10. [Ongoing Concerns](#ongoing-concerns)
11. [Improvement Hypotheses](#improvement-hypotheses)

---

## Iteration #1: Big Upfront Documentation + SpecKit

**Hypothesis:** Create complete application documentation before coding, then use SpecKit to implement piece by piece.

**Method:**
- Used OpenCode (Deepseek V4 Flash) and ChatGPT 5.5 in manual adversarial mode (copy-paste)
- Produced extensive documentation for a near-complete product
- Used GitHub SpecKit to implement incrementally

**Problems encountered:**
- Spent way too much time documenting — the further along I got, the harder it was to determine exact intent, specs became vaguer, and the LLM had more questions
- Implementation with SpecKit: despite massive documentation, I tended to take on too-large chunks (e.g., "create app skeleton" would also pull in SDL3, glm, yaml-cpp instead of focusing on the skeleton's needs)
- I rarely reviewed specs/contracts properly because they were very long, multi-file documents
- `specify.analyze` and `specify.clarify` didn't help me develop a critical eye on specs
- Implementation had frequent ambiguity — LLM made decisions about file locations, naming conventions, etc.
- Creating all documentation upfront proved unviable for LLM-driven development

**Other tools tested:**
- **OpenSpec:** More lightweight but even less review/governance; it rushed to suggest coding and results were often random and incomplete
- **BMad:** The onboarding with personas was nice and helped clarify specs, but after 2 hours of discussion I was still at the "talk to UX expert" stage for an editor I won't build anytime soon — I gave up

**Key takeaways:**
- These experiments gave me a clear vision of the project and confirmed that creating full docs/specs before starting is a bad idea
- As features grew, SpecKit struggled to navigate the codebase
- Manual `/specify` commands in OpenCode are tedious and make the process less agentic — I had to manage the loop between stages manually and often lost track of where we were
- SpecKit generates a lot of boilerplate (scripts, extension files, dozens of commands/skills) — I lost control and couldn't easily customize commands/skills/agents or add steps to the flow
- Git integration (auto-branching) was unreliable: sometimes branches were created, sometimes not, and it added noise to LLM conversations

---

## Iteration #2: Iterative Development with Governance Agents

**Hypothesis:** Return to small incremental development with more governance agents.

**Method:**
- Created custom agents with ChatGPT to build an agentic loop with an orchestrator managing sub-agents (instead of commands)
- Early versions used basic ChatGPT-generated prompts and still produced decent results
- After several iterations, used Deepseek to refine templates and fix observed gaps — results got progressively more precise

**What worked well:**
- The orchestrator agent allowed me to let it work while I did other things; I could read specs while the critic was working
- Giving the orchestrator access to the `question` tool kept me engaged — I often just picked a choice without typing a full answer, and I stayed oriented in the process
- OpenCode's ability to see the LLM working inside sub-agents let me quickly adjust agent prompts as needed
- The orchestrator made the whole exercise more engaging

**Pain and adjustments:**
- Early on, agents struggled to update documentation and create ADRs when needed
- Added dedicated agents for documentation, ADRs, and a governor at the end to ensure cross-document consistency — this saved us repeatedly since we have many files to keep in sync (docs, ADRs, constitution, etc.)

**Problems noticed:**
- Agents often re-read the entire codebase; not an issue initially, but context grew large and LLM calls scaled up
- Realized I needed a wiki for persistent memory

**Constitution addition:**
- Added a rule requiring nearly everything to be tested — this helped significantly

**Remaining issues at this point:**
- Fear of context growth
- Have to restart OpenCode whenever agents are modified
- Full loop can take ~15 minutes per feature — but practically no code rework

---

## Iteration #3: Adding a Wiki for Long-Term Memory

**Hypothesis:** A wiki would provide long-term memory and make it easier to navigate the codebase.

**Method:**
- Built an MCP server backed by a SQLite database with full-text search
- The MCP detects changes via a file watcher and resyncs on restart

**Other adjustments:**
- **Merged Coder and Tester:** The coder must write and run tests anyway; if something fails, all context is already there to fix it. Previously, the coder coded without tests, then the tester created tests and reported failures, forcing the orchestrator to fix issues or the tester to modify the coder's code
- **Actively push agents to use `wiki_search`:** Without explicit prompting, agents preferred reading the codebase directly

**Findings:**
- The wiki + wiki search MCP + dedicated wiki agent significantly improved coordination between features
- **No integration tests:** Multiple times all unit tests passed but nothing rendered on screen when I restarted the app
- Gave most agents access to bash and edit — the orchestrator can now make small spec adjustments without calling a sub-agent, which speeds things up significantly
- The orchestrator tends to write/edit ADRs itself when asked manually — not terrible, but the ADR agent would do a better job, and ideally the orchestrator shouldn't write to disk
- It's interesting that the orchestrator also updates specs and implementation contracts after coding corrections
- Occasionally the orchestrator modified specs/contracts directly and I had to tell it to re-run the critics — despite instructions not to, it sometimes ignores this
- My Code Reviewer does far more than code review (runs tests, etc.) — convenient for now, but not its technical role
- The wiki stopped working for 1–2 features; the scout went back to reading the entire repo. Once the wiki was restored, scout results were more focused and less "I'm re-reading everything"

---

## Iteration #4: Vision MCP for Visual Validation

**Method:**
- Added an MCP for vision-based image analysis
- Took some effort to find a prompt that isn't too detailed but doesn't hallucinate — simple PASS/FAIL doesn't work; the LLM needs to produce explanatory sentences
- Added screenshot capture directly into the buddd CLI

**Outcome:**
- Helped agents fix a screenshot bug (black PNG instead of the expected cube) that they had previously declared functional
- The code agent used it successfully several times

---

## Iteration #5: Coordination File to Reduce Orchestrator Context

**Hypothesis:** A central `coordination.md` file would reduce orchestrator context by avoiding repeated reads/writes of large spec, critic, and contract files.

**Method:**
- Restructured agents around a coordination file as the communication hub between orchestrator and sub-agents
- Modified agents and templates — even used the workflow process to update agents/templates with reasonable success

**Observations:**
- Having ADRs, docs, and a wiki was very helpful to keep the LLM from getting lost
- The orchestrator reads and updates specs/contracts far less directly (though it still does occasionally)
- **Big win:** I can stop the process mid-way and restart it, even in a new session — the orchestrator knows exactly where it left off
- No significant reduction in orchestrator context length though — for a simple feature like "add free camera demo," I was at ~100k context by the time the coder was called

---

## Iteration #6: Grill-Me Step for Deeper Spec Clarification

**Hypothesis:** Adding a "grill me" step would dig deeper into spec clarification, reducing the number of spec iterations needed.

**Method:**
- Added a dedicated prompt and an additional orchestration step in the orchestrator agent

**Observations:**
- Questions surfaced rapidly and pushed analysis further before the spec was even written
- The process is slightly slower, but the questions from DeepSeek V4 were insightful and uncovered blind spots in the original intent
- I asked the agent to write the answers into `coordination.md`, which worked well as a preference
- However, the agent doesn't preserve *why* I made a given choice — if I need to adjust things later, I won't remember the rationale

---

## Iteration #7: Definition of Ready

**Hypothesis:** A shared Definition of Ready checklist — stored in the wiki and referenced by multiple agents — would catch incomplete specs earlier, reduce spec-author→spec-critic loop iterations, and address two persistent gaps: missing E2E verification strategy and undocumented documentation impacts.

**Method:**
- Created `docs/wiki/engineering/definition-of-ready.md` with 13 criteria across 4 categories
- Referenced it in the spec-critic, orchestrator, and spec-author prompts
- Added `## E2E Verification` to spec template; structured `Documentation impact` and split `Required tests` in contract template

**Observations:**
- In the grill me, le LLM asked about e2e testing correctly.


**Still open:**
- The DoR needs real use to validate that criteria are at the right level of detail — too vague and they won't catch anything, too specific and they'll be skipped or cause false positives
- No automated enforcement yet — it relies on agents following their prompts correctly (which the orchestrator doesn't always do, as noted in Iteration #3)
- The `## E2E Verification` section in specs could become a placeholder if spec-authors treat it as a checkbox rather than a meaningful description

---

## Iteration #8: File Update Protocol — Partial Edits Instead of Full Rewrites

**Hypothesis:** Sub-agents that use targeted `edit` instead of full-file `write` will preserve revision history, reduce token waste, and eliminate regression risk from regenerating unchanged sections.

**Problem:**
Every agent (spec-author, implementation-contract-author, spec-critic, implementation-contract-critic, code-reviewer, governance-reviewer) rewrote its entire artifact file from scratch on every invocation — even when only a single blocking issue needed toggling.

This meant:
- A spec-critic re-review that only marks 2 issues as resolved would regenerate all 30+ lines of the review file
- A spec-author correction loop would regenerate 500+ lines of spec.md even if only one section changed
- Previous review history was silently lost because `write` always creates a brand-new file

**Method:**
Added an explicit `## File update protocol` section to all 6 agents, with two distinct modes:

- **Auteurs** (spec-author, implementation-contract-author): `write` for first creation, `edit` ciblé pour les updates. Réécriture complète autorisée uniquement si >50% du fichier change structurellement.
- **Critiques/Reviewers** (spec-critic, implementation-contract-critic, code-reviewer, governance-reviewer): `write` strictement interdit sur un fichier de review existant. Uniquement `edit` pour cocher `[x]`, ajouter des issues, mettre à jour le résumé. La création initiale reste en `write` avec le template.

**Observations:**
- This was previously listed as an untested hypothesis (see Improvement Hypotheses below). Implementing it was straightforward since all agents already had a similar instruction structure.
- The `edit` tool requires exact string matching, which can fail on whitespace differences. The protocol adds: "If edit fails, retry with more surrounding context."
- This change is low-risk — it only modifies agent instructions, not the workflow logic or templates.
- **Orchestrator contradiction caught during review:** The orchestrator itself uses "writes" everywhere in its instructions — the workflow diagram (`spec-author → writes spec.md`), delegation sections ("Ask spec-critic to review and write..."), and loop-back patterns. This directly undermines the File update protocol since the orchestrator primes agents with "write" language. Fixed by:
  - Replacing all `writes` with `creates/updates` in the workflow diagram (6 occurrences)
  - Replacing all `write`/`create` with `create/update` in delegation instructions (6 occurrences)
  - Adding a dedicated paragraph in `Delegation invariant`: "Do NOT ask an agent to 'write' or 'rewrite' its artifact. Say 'update' or 're-review' instead."
- **Human caught it, not the LLM:** This contradiction was pointed out by a human after the initial implementation. The LLM didn't notice that its own orchestrator instructions conflicted with the File update protocol it had just written for the sub-agents — a blind spot worth noting.
- **Systemic pattern:** This isn't an isolated case. The orchestrator has a tendency to give instructions that conflict with sub-agent prompts (e.g., "writes spec.md" vs the sub-agent's "use edit for updates"). It suggests the orchestrator doesn't fully internalize what it delegates — it describes actions from its own perspective (what it wants done) rather than the sub-agent's perspective (how it should do it).

**Still open:**
- Need to verify in practice that agents actually follow the `edit`-first rule rather than falling back to `write` habit.
- The 50% threshold for authors is a judgement call — might need adjustment based on real usage.
- This doesn't address the related hypothesis about skipping template re-reads when updating existing files.
- The orchestrator's tendency to give conflicting instructions is a broader pattern worth monitoring. Other areas where it might contradict sub-agent prompts? (e.g., telling code-implementer to change files vs. telling governance agents not to modify code)

---

- **Orchestrator + sub-agents is a game-changer.** The process is far more autonomous and engaging. Critics suggest improvements, the orchestrator applies them immediately to specs/contracts, and re-runs the critic loop before moving on. This saves a lot of manual spec adjustment.
- **Still missing a way to get more challenging spec/contract critique.** BMad's personas were great, but BMad seems to limit itself to a few user questions.
- **Easy workflow customization is powerful.** Having project-specific agents (e.g., a scout that knows Catchy2 test tags or project structure conventions) makes a real difference.
- **Different features need different workflows.** Being able to change the workflow easily is essential.
- **Parallel feature development is hard** without multiple repo clones.
- **Critic agents at each stage (spec → implementation) remove a lot of ambiguity.** The downside: the human has to read a lot of markdown and can easily skip or approve too quickly. The iterative loop is slower, but rework is drastically reduced — like doing code review twice before writing code.
- **LLMs still make architectural mistakes.** Example: coupling Model and Shader too tightly (passing raw shader text to Model constructor) despite having a Texture concept.
- **The LLM argued about `std::optional<T&>`** — it turned out to be mostly right. We had to create a dedicated ADR.
- **Auto-generated ADRs are great.** The LLM knows to prioritize them.
- **The orchestrator is surprisingly good at recognizing small, simple code requests** and not launching the full workflow.
- **The Wiki Agent never adds new wiki pages.**
- **The governor caught a critical regression:** `*_test.cpp` glob didn't match `*_tests.cpp` — 73 test cases silently dropped from the build. This happened because I asked for manual changes without running the full pipeline. This underscores the importance of automated code review integrated with PRs.
- **Human-in-the-loop is still essential** for catching LLM inconsistencies and nonsensical architectural decisions.
- **I rarely look at the code anymore.** The implementation contract contains almost everything I need to know.
- **I've become more interested in optimizing the process and watching how the LLM handles the task than in the code or whether it actually works.** I feel very detached from both the code and the end result. I'm not sure if this is because the project itself is about testing a SDD process, but I wonder if this would carry over to a "normal" project.

---

## Ongoing Concerns

- The wiki grows organically and will eventually become huge — it may become hard to navigate
- Wiki, ADR, and constitution are difficult to keep in sync, even for LLMs. Small inconsistencies slip in
- Restructuring the wiki after organic growth will be complex
- Reconstructing wiki + ADRs from a large codebase (if needed) would be a challenge
- **No task decomposition** (unlike GitHub SpecKit) — the Code Implementer does everything at once. Current results are excellent, but the human must use experience and feel to decide what's too small or too big. Backlog management and task splitting are still manual. This approach won't scale to very large features — and I personally think building very large features in one go is a bad idea anyway
- The orchestrator's context grows fast (often >100k even for small features). Smaller models would hallucinate a lot. I've sometimes had to stop at human approval and start a fresh discussion
- Documentation and governance agents don't do a great job maintaining project docs. For example, when the `--capture` argument was removed from the CLI, the README was never updated
- Early specs often don't define how we'll verify the feature actually works end-to-end (e.g., adding a demo/app for real testing). This should be part of the grill-me step or formalized as a definition-of-ready checklist that the spec-critic enforces
- The agents have some instructions that are specific to buddd engine, these should be included in docs folder and referenced by the agents.
- New specs consistently include a `## Status` and `## Approval` section (with Draft/In Review/Accepted and an approval table) even though neither section exists in the spec template — this seems to be a learned behavior carried over from earlier iterations or from the coordination.md format, and it adds unnecessary bloat to every spec file.
- **Stale specs:** Specs are written once during the workflow and never updated afterward. As the code evolves through subsequent features and refactors, the spec becomes increasingly inaccurate. The LLM has no way to distinguish what in the spec is still true and what is outdated — it trusts the spec (authority order #2), which leads to confusion and incorrect decisions.
- **Wiki contaminated by stale spec references:** The wiki references spec IDs and details extensively in its architecture, domain, and decision pages. When a spec goes stale, every wiki page referencing it becomes partially wrong too. The LLM then blends outdated spec details with possibly-correct wiki content, producing a confusing hybrid that's hard to debug. Over time, the wiki becomes a tangled web of cross-references to increasingly inaccurate specs, making it harder to maintain — not easier. I suspect the solution isn't better wiki maintenance but rather **decoupling the wiki from spec references entirely**: the wiki should describe current state only, and spec references should be limited to ADRs (which capture decisions, not evolving behavior).
- **No distinction between ADRs and standards:** The current framework treats ADRs and constitution rules as separate governance documents, but there's no clear criterion for when something should be an ADR vs a constitution rule vs just a wiki convention. This causes two problems: (1) too many ADRs are created — every architectural decision gets an ADR, even trivial ones that would be better as a quick wiki note or a constitution rule; (2) ADRs become a dumping ground that mixes historical decisions with active standards, making it hard to know which ADRs are still relevant. An ADR should capture a **decision with rationale** (why we chose X over Y, context, tradeoffs). A constitution rule should capture an **active constraint** (thou shalt not include X headers). A wiki page should capture **current understanding** (how the module works today). When these blur, LLMs (and humans) don't know where to look for what.
- **Root README.md is nobody's responsibility:** The root `README.md` is never updated during development. Two root causes identified: (1) the wiki agent is restricted to `docs/wiki/**` and cannot touch it; (2) no other agent in the workflow has permission or mandate to maintain it — the implementation contract has a `Documentation impact` section that mentions README, but there's no agent that both can and must act on it. Additionally, there are two README files (`/README.md` and `docs/wiki/README.md`), only the latter is indexed by wiki search, making the root one invisible to agents during research.

## Improvement Hypotheses

- **Clarify governance/testing/code-review roles:** The governor currently runs tests at the end — not ideal. Better separation would help
- **Use different models for critics:** Specialized models might give better critique
- **Integration tests with vision:** Either a vision model or a dedicated image-validation sub-agent
- **Wiki reorganization agent:** Prevent wiki pages from becoming catch-all dumping grounds
- **Maintainability validation agent:** Catch architectural inconsistencies and bad practices
- **Reduce orchestrator context growth:** Find a way for the orchestrator to avoid reading full specs. A small communication file where agents post questions, set states, etc., could be more efficient than the current coordination model
- **Generic screenshot tool:** Instead of app-specific capture code, a general screenshot mechanism would let agents test any UI without custom code
- **Human-in-the-loop review before/after code review:** Currently I have to stop the process or wait until the end to test — and features often don't work 100% (e.g., inverted mouse movement in the free-camera demo)
- ~~**Partial edits instead of full rewrites:** Currently sub-agents rewrite the entire spec, contract, or review file on every iteration. This burns context and risks losing details that weren't flagged as issues. An alternative would be to instruct agents to make targeted edits — append new sections, update specific paragraphs, mark resolved items — rather than regenerating the whole document from scratch.~~ ✅ **Implemented in Iteration #8** — added `## File update protocol` to all 6 writer agents, enforcing `edit` over `write` for updates.
- **Avoid re-reading templates on every invocation:** Sub-agents load their template file at the start of every task, even when the artifact already exists and only needs updating. This wastes context and tokens. An alternative would be to let the agent skip template loading when the target file already exists and only load the template for brand-new documents.
- **Spec staleness detection or auto-update:** Specs are written once and never updated, so they drift from reality as the code changes. Possibilities include: treating specs as snapshots tied to a git ref, having a dedicated agent that audits and updates stale specs, or deprecating specs after implementation and relying solely on the wiki + code for ground truth.
- **Clearer ADR vs standard vs convention boundaries:** Create explicit criteria for what goes where. An ADR captures a decision with rationale (why X over Y, context, tradeoffs, date, author). A constitution rule captures an active constraint (must/must not, applies globally). A wiki convention captures current understanding (how things work, patterns, non-binding recommendations). If the LLM can't decide which bucket something falls into, it should escalate rather than default to creating an ADR.