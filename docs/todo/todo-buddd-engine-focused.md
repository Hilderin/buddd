# Buddd Engine — Focused Todo

## Product Focus

Buddd Engine should first prove that it can be used to create a small playable 3D game from its own editor.

The first real milestone is not “build a complete AA game engine”.
The first real milestone is:

> **Milestone 1 — Playable FPS From Editor**

The goal is to create a tiny FPS prototype using Buddd Editor, while writing gameplay code externally in C++.

The editor is part of MVP1, but only as a scene composition tool. It is not yet an IDE, not yet a script editor, and not yet a full Unity/Godot-like environment.

---

# Guiding Principles

## 1. Build a vertical slice, not a feature encyclopedia

Every feature in MVP1 should support the same concrete test:

> Can I create, save, reload, and play a small FPS scene from the editor?

If a feature does not directly help that test, defer it.

## 2. Editor early, but tiny

The editor should exist early because it forces the engine to solve real workflow problems:

- scene hierarchy
- component inspection
- asset references
- transform editing
- scene save/load
- play mode
- game module integration

But the editor should stay minimal. No code editor, no debugger, no visual scripting, no shader graph, no complex plugin system yet.

## 3. Gameplay code lives outside the editor for MVP1

Gameplay code is written in an external C++ project using Visual Studio, CMake, or another normal development environment.

Buddd Editor does not compile code in MVP1.
Buddd Editor only loads/reloads the compiled game module.

Example project layout:

```txt
BudddEngine/
BudddEditor/
BudddRuntime/
GameProject/
  src/
    PlayerController.cpp
    Weapon.cpp
    Health.cpp
    Door.cpp
  assets/
  scenes/
```

Example gameplay module:

```txt
GameProject.dll
```

The editor should expose C++ components registered by the game module.

Example:

```cpp
REGISTER_COMPONENT(PlayerController);
REGISTER_COMPONENT(Weapon);
REGISTER_COMPONENT(Health);
```

The inspector can edit reflected properties:

```cpp
REFLECT_PROPERTY(moveSpeed);
REFLECT_PROPERTY(mouseSensitivity);
REFLECT_PROPERTY(jumpForce);
```

---

# Milestone 1 — Playable FPS From Editor

## Goal

Create a small playable FPS prototype from Buddd Editor.

The user should be able to:

1. Open Buddd Editor.
2. Create a new scene.
3. Add a player entity.
4. Add an FPS controller component.
5. Add a camera.
6. Add basic meshes or cubes.
7. Add colliders.
8. Save the scene.
9. Reload the scene.
10. Press Play.
11. Move, look around, jump, and shoot using raycasts or simple projectiles.

---

## MVP1 Included Scope

### 1. Developer foundation

#### 1.1 Logging system
Status: Done  
Priority: Critical

Implement structured logging with levels and channels.

Required for MVP1:

- log levels: trace, debug, info, warn, error, fatal
- channels: engine, editor, render, asset, input, physics, game
- console output
- file output
- in-memory ring buffer for editor console/debug panel

Do not overbuild rotating logs or remote log upload yet.

---

#### 1.2 Assertions and error reporting
Status: Done  
Priority: Critical

Add engine assertions with file, line, message, and optional breakpoint in debug builds.

Required for MVP1:

- `BUDDD_ASSERT(condition, message)`
- `BUDDD_VERIFY(condition, message)`
- crash-friendly error logs
- clear failure messages during editor/runtime development

Deferred:

- Breakpad
- crash upload
- cloud diagnostics

---

#### 1.3 Config and cvars minimal
Status: Not started  
Priority: High

Add a minimal runtime configuration system.

Required for MVP1:

- load config from `engine.ini` or project config
- expose cvars for common settings
- allow runtime editing through debug panel
- persist changed settings when needed

Example cvars:

```txt
r.vsync true
r.clear_color 0.1 0.1 0.1
input.mouse_sensitivity 0.15
physics.debug_draw false
editor.show_grid true
```

Deferred:

- graphics preset system
- remote config
- full command console

---

#### 1.4 ImGui editor shell and debug overlay
Status: Not started  
Priority: Critical

Integrate Dear ImGui as the basis for the editor and debug tools.

Required for MVP1:

- docking
- main menu bar
- editor panels
- FPS/frame time overlay
- log panel
- cvar/debug panel

This is the UI foundation for the MVP1 editor.

---

# 2. Runtime core

## 2.1 Entity/component foundation cleanup
Status: Not started  
Priority: Critical

Stabilize the existing ECS/component model enough for editor usage.

Required for MVP1:

- create/destroy entities
- add/remove components
- query components
- entity names
- stable entity IDs
- enabled/disabled state
- parent/child hierarchy support

Deferred:

- high-performance archetype ECS
- DOTS-like chunk storage
- jobified ECS iteration

---

## 2.2 Transform hierarchy
Status: Not started  
Priority: Critical

Implement parent-child transform propagation.

Required for MVP1:

- local position/rotation/scale
- world matrix calculation
- parent/child relationships
- reparenting while preserving world transform
- transform dirty propagation

This is required for editor hierarchy, cameras, player rigs, weapons, doors, and scene organization.

---

## 2.3 Reflection/metadata for components
Status: Not started  
Priority: Critical

Create a small reflection layer so the editor can display and edit component properties.

Required for MVP1:

- register component types
- register properties
- property name
- property type
- get/set property value
- serialize property values
- draw default inspector widgets for common types

Required property types:

- bool
- int
- float
- string
- Vec2
- Vec3
- Vec4
- Color
- Entity reference
- Asset reference

Deferred:

- full C++ reflection generator
- scripting metadata
- custom property drawers
- advanced serialization attributes

---

## 2.4 Scene serialization
Status: Not started  
Priority: Critical

Save and load complete editor scenes.

Required for MVP1:

- serialize entities
- serialize names and hierarchy
- serialize components
- serialize reflected properties
- serialize asset references
- load scene back into the same state

Recommended format for MVP1:

- human-readable JSON or YAML

Deferred:

- binary scene format
- schema migrations
- prefab override serialization
- additive scene streaming

---

## 2.5 Resource and asset reference system minimal
Status: Not started  
Priority: Critical

Create a basic resource registry so scenes can reference assets reliably.

Required for MVP1:

- asset path or asset ID
- load glTF mesh assets
- load textures
- load materials or material parameters
- handle missing asset gracefully
- display assets in the editor asset browser

Deferred:

- full import pipeline
- baking
- compression
- bundle splitting
- live update
- encrypted archives

---

# 3. Rendering needed for the FPS slice

## 3.1 Stable mesh rendering path
Status: Partially started  
Priority: Critical

Make the current mesh rendering path robust enough for editor and runtime play mode.

Required for MVP1:

- render glTF meshes
- render primitive debug meshes/cubes
- render materials
- support camera selection
- support editor viewport render target
- support game viewport render target

Deferred:

- render graph
- clustered lighting
- GI
- ray tracing
- advanced post-processing

---

## 3.2 Basic lighting
Status: Not started  
Priority: High

Support enough lighting to make the FPS scene readable.

Required for MVP1:

- directional light
- point light
- ambient/environment fallback
- editable light component in inspector

Deferred:

- shadows
- SSAO
- volumetric fog
- light probes
- reflection probes

---

## 3.3 Debug drawing
Status: Not started  
Priority: High

Add simple debug drawing for editor and gameplay debugging.

Required for MVP1:

- lines
- boxes
- spheres
- rays
- transform axes
- physics collider visualization

This is especially important before advanced editor tooling exists.

---

# 4. Input and gameplay loop

## 4.1 Input action system minimal
Status: Not started  
Priority: Critical

Abstract raw SDL input into gameplay/editor actions.

Required for MVP1:

- keyboard
- mouse
- action bindings
- axis bindings
- mouse delta
- editor input context
- gameplay input context

Example actions:

```txt
MoveForward = W
MoveBackward = S
MoveLeft = A
MoveRight = D
Jump = Space
Shoot = MouseLeft
Look = MouseDelta
```

Deferred:

- rebinding UI
- gamepad support
- touch input
- multiple players
- complex input contexts

---

## 4.2 Game loop and play mode separation
Status: Not started  
Priority: Critical

Separate editor mode from play mode.

Required for MVP1:

- edit mode
- play mode
- stop play mode
- clone or reload scene for play mode
- restore editor scene after stopping
- update game components only during play mode

Deferred:

- pause/frame-step
- hot reload during play
- edit-and-continue
- play mode state diffing

---

# 5. Physics needed for FPS

## 5.1 Physics backend minimal
Status: Not started  
Priority: Critical

Integrate a physics backend, likely Jolt, but only expose the minimum needed for the FPS slice.

Required for MVP1:

- physics world
- fixed timestep
- static body
- dynamic rigid body
- capsule or character collider
- box collider
- sphere collider
- raycast
- collision layers minimal

Deferred:

- vehicles
- cloth
- soft bodies
- ragdolls
- joints beyond what is required
- navmesh
- complex query API

---

## 5.2 Character controller minimal
Status: Not started  
Priority: Critical

Implement a simple FPS character controller.

Required for MVP1:

- move on ground
- jump
- gravity
- slope handling minimal
- collide with walls/floor
- configurable speed
- configurable jump force

This can be implemented as a C++ gameplay component using the physics API.

---

## 5.3 Raycast shooting
Status: Not started  
Priority: High

Implement a simple weapon interaction.

Required for MVP1:

- raycast from camera center
- hit detection
- debug ray visualization
- optional impact marker
- call `OnHit` or apply damage to a `Health` component

Deferred:

- projectiles
- recoil
- bullet decals
- weapon animation
- inventory system

---

# 6. Editor MVP

## 6.1 Editor application shell
Status: Not started  
Priority: Critical

Create the basic Buddd Editor executable.

Required for MVP1:

- open project folder
- main menu
- dockable panels
- load/save scene
- play/stop button

Deferred:

- project templates
- package manager
- plugin manager
- editor themes beyond basic dark theme

---

## 6.2 Scene hierarchy panel
Status: Not started  
Priority: Critical

Display scene entities in a tree.

Required for MVP1:

- show entity names
- show parent/child hierarchy
- select entity
- create entity
- delete entity
- reparent entity

Deferred:

- advanced search
- multi-select
- drag/drop polish
- prefab indicators

---

## 6.3 Inspector panel
Status: Not started  
Priority: Critical

Edit selected entity components and properties.

Required for MVP1:

- show selected entity name
- edit Transform
- add/remove components
- edit reflected component properties
- assign asset references

Deferred:

- custom inspectors
- property validation system
- animation of properties
- nested prefab overrides

---

## 6.4 3D viewport
Status: Not started  
Priority: Critical

Render the scene into an editor viewport.

Required for MVP1:

- editor camera
- orbit/pan/dolly or FPS-style editor navigation
- select entity from hierarchy
- focus selected entity
- display grid
- display debug draw

Deferred:

- mouse picking, unless easy
- multi-viewport
- device simulator
- scene view effects

---

## 6.5 Transform gizmo minimal
Status: Not started  
Priority: High

Allow moving objects visually.

Required for MVP1:

- translate gizmo
- rotation/scale can be inspector-only initially
- snapping optional

Recommended shortcut:

Use ImGuizmo if compatible with the current stack.

Deferred:

- full translate/rotate/scale polish
- pivot modes
- local/global toggle
- surface snapping

---

## 6.6 Asset browser minimal
Status: Not started  
Priority: High

Show project assets and allow assigning/using them.

Required for MVP1:

- browse project `assets/` folder
- show basic file list
- drag or assign mesh/texture/material references
- reload asset on demand

Deferred:

- thumbnails
- metadata database
- import status
- asset refactoring
- rename propagation

---

# 7. Game module integration

## 7.1 Game module API
Status: Not started  
Priority: Critical

Define how external gameplay code plugs into the engine.

Required for MVP1:

- game module entry point
- component registration
- system registration if needed
- lifecycle hooks
- safe unload/reload boundary if feasible

Example:

```cpp
extern "C" void Buddd_RegisterGame(Buddd::GameRegistry& registry)
{
    registry.RegisterComponent<PlayerController>();
    registry.RegisterComponent<Weapon>();
    registry.RegisterComponent<Health>();
}
```

Deferred:

- native plugin SDK
- ABI stability promises
- marketplace-style extensions
- scripting bindings

---

## 7.2 Manual game module reload
Status: Not started  
Priority: High

Allow the editor to reload the compiled game module.

Required for MVP1:

- unload current module when not in play mode
- load new DLL/shared library
- refresh registered component types
- preserve scene data when possible
- show errors clearly if reload fails

Important MVP1 constraint:

Buddd Editor does not compile the module. The developer compiles externally.

Deferred:

- automatic hot reload
- live reload while playing
- state migration across DLL reload
- integrated build button

---

# 8. FPS demo content

## 8.1 FPS controller component
Status: Not started  
Priority: Critical

A gameplay component in the sample game module.

Required for MVP1:

- WASD movement
- mouse look
- jump
- configurable speed
- configurable mouse sensitivity
- uses input action system
- uses physics character controller or capsule body

---

## 8.2 Camera component / active camera
Status: Not started  
Priority: Critical

Support an active runtime camera.

Required for MVP1:

- camera component
- FOV
- near/far planes
- active camera selection
- camera attached to player

Deferred:

- Cinemachine-like virtual cameras
- camera blending
- camera shake framework

---

## 8.3 Weapon component minimal
Status: Not started  
Priority: High

A simple shooting interaction for the FPS demo.

Required for MVP1:

- shoot action
- raycast from camera
- debug line or impact marker
- optional damage to target

---

## 8.4 Health / damage component
Status: Not started  
Priority: Medium

A tiny gameplay component to prove component interaction.

Required for MVP1:

- health value
- apply damage
- destroy or disable entity at zero

---

## 8.5 Demo scene
Status: Not started  
Priority: Critical

Create a sample scene that proves the whole pipeline.

Required for MVP1:

- floor
- walls
- player start
- lights
- several target objects
- colliders
- saved scene file
- playable from editor

---

# MVP1 Definition of Done

Milestone 1 is complete when this workflow works end-to-end:

1. Launch Buddd Editor.
2. Open a sample project.
3. Create or open `fps_demo.scene`.
4. See entities in the hierarchy.
5. Select an entity and edit its components in the inspector.
6. Move at least one object in the viewport.
7. Assign a mesh or material asset.
8. Save the scene.
9. Close and reopen the editor.
10. Reload the scene successfully.
11. Press Play.
12. Control the player in first person.
13. Collide with the environment.
14. Shoot a raycast weapon.
15. Stop play mode and return to the editable scene.

---

# Explicit MVP1 Exclusions

These are intentionally not part of MVP1:

- integrated code editor
- script debugger
- Lua scripting
- visual scripting
- shader graph
- VFX graph
- prefab inheritance
- nested prefabs
- advanced undo/redo
- animation editor
- terrain editor
- UI builder
- audio buses/effects
- multiplayer
- cloud services
- ads/IAP
- Web/WASM
- mobile
- XR
- consoles
- ray tracing
- GI
- advanced post-processing
- job system
- high-performance ECS rewrite
- modding
- ML agents

---

# Recommended Execution Order for MVP1

## Phase A — Make the engine observable

1. Logging
2. Assertions
3. ImGui shell
4. Debug overlay
5. Config/cvars minimal

## Phase B — Make scenes real

6. Entity IDs/names/hierarchy
7. Transform hierarchy
8. Component reflection metadata
9. Scene serialization
10. Asset references

## Phase C — Make the editor useful

11. Editor application shell
12. Hierarchy panel
13. Inspector panel
14. Viewport panel
15. Transform gizmo minimal
16. Asset browser minimal

## Phase D — Make gameplay possible

17. Input action system
18. Play mode separation
19. Physics backend minimal
20. Character controller minimal
21. Runtime active camera

## Phase E — Prove the FPS slice

22. Game module API
23. Manual game module reload
24. FPS controller component
25. Weapon/raycast component
26. Health/damage component
27. Demo FPS scene

---

# Milestone 2 — Better Authoring Workflow

Start only after MVP1 is fully usable.

Goal:

> Make building scenes less painful and less fragile.

Likely scope:

- undo/redo minimal
- prefab system basic
- better asset browser
- mouse picking in viewport
- rotate/scale gizmos
- material editor minimal
- editor camera polish
- project creation/opening flow
- simple audio playback
- basic UI text/button rendering
- simple build/export prototype

Still deferred:

- scripting
- code editor
- debugger
- visual scripting

---

# Milestone 3 — Scripting and Iteration Speed

Goal:

> Let gameplay be authored faster without recompiling C++ for every small change.

Possible scope:

- Lua scripting
- script component
- script hot reload
- script errors in editor console
- external editor integration
- generated API docs/stubs for autocomplete
- optional VS Code workspace generation

Important:

Still do not build a full integrated code editor unless there is a strong reason.

Recommended approach:

- use VS Code / Visual Studio externally
- Buddd generates project files or stubs
- Buddd watches scripts and reloads them
- Buddd shows script errors in the editor console

---

# Milestone 4 — Debugging and Profiling

Goal:

> Make projects debuggable and optimizable.

Possible scope:

- CPU profiler integration, likely Tracy
- memory tracking
- GPU timing queries
- frame/debug capture basics
- script debugger if Lua exists
- custom performance counters
- better in-game overlay

---

# Milestone 5 — Production Runtime Basics

Goal:

> Make exported games possible.

Possible scope:

- asset baking
- resource packing
- standalone runtime export
- project manifest
- platform configuration
- launcher/config dialog
- deterministic build pipeline basics

---

# Milestone 6 — Gameplay Feature Expansion

Goal:

> Add systems needed by real small games.

Possible scope:

- skeletal animation
- animation state machine minimal
- audio engine
- UI system
- save/load game state
- localization
- particles basic
- navmesh/pathfinding if needed
- camera system polish

---

# Long-Term Backlog

These belong in the long-term backlog, not the near-term roadmap:

- advanced rendering: GI, ray tracing, SDFGI, DLSS/XeSS
- visual shader graph
- visual VFX graph
- full terrain system
- cloud services
- IAP/ads
- multiplayer stack
- voice chat
- anti-cheat
- Web/WASM
- mobile
- XR
- console support
- modding platform
- ML agents
- advanced destruction
- full editor plugin ecosystem

---

# One-Sentence Strategy

Buddd Engine should first become a tiny Unity-like editor capable of producing one simple FPS scene, not a complete Unity/Godot competitor.
