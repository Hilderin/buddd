# Buddd Engine

[![CI](https://github.com/Hilderin/buddd/actions/workflows/ci.yml/badge.svg)](https://github.com/Hilderin/buddd/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++26](https://img.shields.io/badge/C%2B%2B-26-00599C.svg)](https://en.cppreference.com/w/cpp/26)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](http://makeapullrequest.com)

**Buddd** is a C++26 3D graphics engine and the living testbed for a custom **Spec-Driven Development (SDD)** framework built on top of [OpenCode](https://opencode.ai).

> **Spec-Driven Development** means that every line of production code is derived from an accepted functional spec, refined through a multi-agent critique pipeline, and gated behind a rigorous workflow — no code is written directly from a raw user request or an unvalidated spec.

---

## The Big Idea

Buddd is two things in one repository:

### 1. A 3D Graphics Engine

A cross-platform C++26 engine with SDL3 + OpenGL rendering, built with modern C++ patterns:

- **Dual backend architecture** — SDL3 (display) and Headless (no display) backends for Platform, Window, RenderDevice, and InputSystem
- **OpenGL render pipeline** — shader compilation, material system, vertex/index buffers, texture loading, Phong lighting
- **Scene graph** — entity-component system with a typed component API, transform hierarchy, and camera registration
- **Math library** — zero-overhead GLM wrapper types (`Vec3`, `Mat4`, `Quat`, `Camera`) with `reinterpret_cast`-based interop
- **CLI interface** — commands for running demos (triangle, cube, textured cube, Phong lighting, free camera, scene), capture (screenshot/framebuffer readback), and version info
- **Comprehensive test suite** — Catch2 unit tests covering math, rendering, scene graph, input, lighting, platform abstraction, and more
- **CI-ready** — Docker-based CI pipeline with GCC 16, CMake presets, and headless test execution

### 2. A Spec-Driven Development Framework

The real innovation is the **multi-agent SDD workflow** that governs how the engine itself is built:

```
Human Intent
  → Orchestrator (clarification)
  → Spec Author (drafts functional spec)
  → Spec Critic (validates spec)
  → Implementation Contract Author (converts spec to precise contract)
  → Implementation Contract Critic (validates contract)
  → Human Validation Gate
  → Code Implementer (writes code + tests)
  → Code Reviewer (reviews implementation)
  → ADR / Constitution / Wiki updates
  → Governance Reviewer (cross-document validation)
  → Done
```

Each step is performed by a specialised OpenCode agent defined in `.opencode/agents/`:

| Agent | Role |
|---|---|
| `orchestrator` | Main human interface; coordinates the full workflow |
| `scout` | Targeted reconnaissance of code, wiki, ADRs, constitution |
| `spec-author` | Drafts functional specs from human intent |
| `spec-critic` | Validates specs for ambiguity, testability, scope |
| `implementation-contract-author` | Converts accepted specs into precise contracts |
| `implementation-contract-critic` | Validates implementation contracts |
| `code-implementer` | Implements accepted contracts only |
| `code-reviewer` | Reviews code against spec, contract, tests, constitution |
| `adr-agent` | Proposes ADRs for architectural decisions |
| `constitution-agent` | Maintains project constitution rules |
| `wiki-agent` | Maintains operational wiki |
| `governance-reviewer` | Final cross-document governance validation |

This workflow enforces a strict **authority order** for all project decisions:

1. `docs/constitution/` — mandatory project rules
2. `docs/specs/` — functional specs (proposed + accepted)
3. `docs/adr/` — architectural decision records
4. `docs/wiki/` — operational knowledge
5. Existing code conventions

## Why?

Most AI-assisted code generation bypasses specification entirely — you ask for a feature and get code. Buddd explores the opposite extreme: a heavily gated, multi-agent pipeline where every change is specified, critiqued, contracted, approved, implemented, reviewed, and documented before it lands. This is an experiment in **deterministic, auditable, and reviewable AI-generated code**.

The project's SpecKit (`SpecKit.md`) and `AGENTS.md` document the full workflow and can be reused as a **Spec-Driven Development starter kit** for other projects.

> **Model:** All agents run on DeepSeek V4 Flash via OpenCode. See [experiments-spec-driven-dev.md](experiments-spec-driven-dev.md) for the full experiment log across 5 iterations of this workflow.

---

## Build

### Prerequisites

- CMake >= 3.28
- Ninja
- GCC 16+ or Clang 22+
- OpenGL development libraries

### MCP tools (required for the SDD workflow)

The multi-agent workflow relies on two MCP servers that must be cloned as siblings to `buddd`:

```
/parent/
├── buddd/               # this repo
├── llm-wiki-search/     # https://github.com/Hilderin/llm-wiki-search
└── llm-openai-vision-mcp/  # https://github.com/Hilderin/llm-openai-vision-mcp
```

- [llm-wiki-search](https://github.com/Hilderin/llm-wiki-search) — provides wiki search capabilities for agents (configured in `opencode.json` → `mcp.wiki`)
- [llm-openai-vision-mcp](https://github.com/Hilderin/llm-openai-vision-mcp) — provides vision analysis for visual regression checks (configured in `opencode.json` → `mcp.vision`)

You also need an OpenAI API key saved to `.secrets/openapi.key` for the vision MCP server.

### Quick start

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

### Headless mode (no display)

```bash
cmake --preset debug -DBUDDD_HAS_DISPLAY=OFF
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

### Docker CI

```bash
docker build -f docker/ci.Dockerfile -t buddd-ci:latest .
docker run --rm -v $(pwd):/workspace -w /workspace buddd-ci:latest
```

---

## Usage

```bash
# Run the default engine demo
./build/debug/src/cmd/buddd

# List available demos
./build/debug/src/cmd/buddd help

# Run a specific demo
./build/debug/src/cmd/buddd demo <demo-name>

# Capture a screenshot
./build/debug/src/cmd/buddd capture <output.png>
```

---

## Project Structure

```
buddd/
├── src/
│   ├── engine/          # Core engine library
│   │   ├── render/      # OpenGL + headless render pipeline
│   │   ├── platform/    # SDL3 + headless platform abstraction
│   │   ├── window/      # SDL3 + headless window abstraction
│   │   ├── input/       # SDL3 + headless input system
│   │   ├── scene/       # Entity-component scene graph
│   │   ├── math/        # GLM wrapper types (Vec3, Mat4, Quat, Camera)
│   │   └── image/       # Image loading (stb)
│   ├── cmd/             # CLI binary
│   │   ├── commands/    # run, demo, capture, version, help
│   │   ├── demo/        # Demo scenes (triangle, cube, Phong, etc.)
│   │   └── capture/     # Screenshot capture scenes
│   └── editor/          # Editor library (placeholder)
├── tests/               # Catch2 unit tests
├── docs/
│   ├── constitution/    # Mandatory project rules
│   ├── specs/           # Functional specs + implementation contracts
│   ├── adr/             # Architecture Decision Records
│   ├── wiki/            # Operational knowledge
│   └── templates/       # Document templates
├── docker/              # CI Dockerfile
├── .opencode/agents/    # OpenCode SDD multi-agent definitions
├── SpecKit.md           # Spec-Driven Development starter kit
└── AGENTS.md            # Agent operating rules
```

---

## License

MIT — see [LICENSE](LICENSE).

---

*Built with curiosity, C++ and exclusively by AI Agents.*