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
7. [Cross-Cutting Observations](#cross-cutting-observations)
8. [Ongoing Concerns](#ongoing-concerns)
9. [Improvement Hypotheses](#improvement-hypotheses)

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

## Cross-Cutting Observations

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

---

## Improvement Hypotheses

- **Clarify governance/testing/code-review roles:** The governor currently runs tests at the end — not ideal. Better separation would help
- **Use different models for critics:** Specialized models might give better critique
- **Integration tests with vision:** Either a vision model or a dedicated image-validation sub-agent
- **Wiki reorganization agent:** Prevent wiki pages from becoming catch-all dumping grounds
- **Maintainability validation agent:** Catch architectural inconsistencies and bad practices
- **Reduce orchestrator context growth:** Find a way for the orchestrator to avoid reading full specs. A small communication file where agents post questions, set states, etc., could be more efficient than the current coordination model
- **Generic screenshot tool:** Instead of app-specific capture code, a general screenshot mechanism would let agents test any UI without custom code
- **Human-in-the-loop review before/after code review:** Currently I have to stop the process or wait until the end to test — and features often don't work 100% (e.g., inverted mouse movement in the free-camera demo)
