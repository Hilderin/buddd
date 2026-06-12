# Scene Management

## Current state

The Buddd Editor follows an **Unreal Engine-like** model: one scene open at a time in the dedicated Scene tab. All scene file operations (New, Open, Save, Save As, Quit) go through the **File** menu. The engine's `SceneLoader` and `SceneSaver` APIs handle YAML serialization/deserialization of entity hierarchies.

## How it works

### File Menu Operations

| Operation | Trigger | Behavior |
|---|---|---|
| **New Scene** | File > New Scene | Creates empty, untitled scene. Prompts save if current scene is dirty. Clears the World. |
| **Open Scene** | File > Open Scene | Opens OS file dialog (`.yaml` filter). Loads selected YAML via `SceneLoader`. Prompts save if current scene is dirty. |
| **Save Scene** | File > Save Scene | If scene has a file path, saves via `SceneSaver`. If untitled, behaves like Save As. Clears dirty indicator. |
| **Save Scene As** | File > Save Scene As | Opens OS file dialog. Saves scene to chosen path. Updates scene's file path. |
| **Quit** | File > Quit | Prompts save if any open tab (Scene or Prefab) is dirty. Exits application. |

### Dirty State

- The scene is **dirty** if any entity, component, or property has been modified since the last save.
- The Scene tab title shows an asterisk (`*`) when dirty (e.g., `🔷 *Scene_01`).
- Prefab tabs also track **independent** dirty state.
- Saving clears the dirty state. If the same prefab is open in multiple tabs, saving one tab updates the file but the other tab's dirty state is unaffected.

### Scene Replacement Workflow

When opening a different scene (via File > Open Scene or double-clicking a scene in the Project panel) with a dirty current scene:

1. A modal dialog appears: **"Save changes to [scene name]?"**
2. Three options:
   - **Save** — The current scene is saved, then the new scene loads.
   - **Don't Save** — The current scene is discarded, new scene loads.
   - **Cancel** — The operation is aborted, current scene remains open.

### Untitled Scene Behavior

- A newly created scene (File > New Scene) is **untitled** — it has no file path.
- The tab title shows "Untitled*" (dirty by default).
- File > Save on an untitled scene opens the Save As dialog.
- File > Save Scene As is always available and assigns a file path.

### Scene File Format

Scenes are serialized as `.yaml` files via `SceneSaver` and loaded via `SceneLoader`. The YAML format includes entity hierarchy, component data, and type metadata.

## Important conventions

- Only one scene is open at a time in the Scene tab. Opening another replaces the current scene.
- File operations use **OS-native file dialogs** (not an editor-implemented browser).
- Scene file load errors (corrupt/missing YAML) show an error dialog and preserve the current scene.
- Save failures (disk full, permissions) show an error dialog; the scene remains dirty.
- All editor file operations are logged to the Console via the `Editor:Scene` log channel.

## Domain Concepts

| Concept | Description |
|---|---|
| **Scene** | A YAML file containing a complete entity hierarchy with all components and their properties. |
| **SceneLoader** | Engine API that parses YAML scene/prefab files and populates a `World` with entities and components. |
| **SceneSaver** | Engine API that serializes a `World` back to YAML, respecting entity source types (scene entities expanded, prefab/model entities referenced). |
| **World** | Container holding all entities, their hierarchy, and the component registry. |

## Related specs

- [SPEC-2026-06 — Editor UX Design (North-Star)](/.specs/sprint-2026-06/editor-ux-design/spec.md) — Complete editor UX design document (Scene Management section)

## Related ADRs

- [ADR-027](/docs/adr/ADR-027-editor-architecture.md) — Editor architecture: separate static library, EditorApp adapter, namespace, architecture boundary
- [ADR-029](/docs/adr/ADR-029-editor-ux-decisions.md) — Editor UX decisions (panel layout, tab system, play mode)
- [ADR-014](/docs/adr/ADR-014-cli-app-system.md) — CLI App System (editor uses `run_app()`)

## Last reviewed

2026-06-11 — Created from SPEC-2026-06 (Editor UX Design)
