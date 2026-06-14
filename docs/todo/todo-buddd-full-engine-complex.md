# Buddd Engine — Roadmap to AA 3D Game Engine

> Comprehensive roadmap synthesized from Godot 4.6, Defold 1.12, and Unity 2022.3 feature sets.
> Total: ~204 features across 17 phases.

Current engine state: **~3% complete** (v0.1.0 — C++26, OpenGL 4.5 Core, ECS, glTF assets, Phong+PBR, headless backend, CLI demos)

---

## High-Level Roadmap

```
Phase  0  ██  Developer Foundation        — log, config, imgui, debug overlay
Phase  1  ██  Core Data Architecture      — tags, prefabs, scenes, input actions, messaging
Phase  2  ██  Audio                       — 3D spatial, buses, streaming
Phase  3  ██  Physics + Navigation        — Jolt, collisions, navmesh
Phase  4  ██  Animation                   — skeletal, state machine, IK, rigging
Phase  5  ██  Rendering Advanced          — shadows, post-process, GI, sky, fog, compute
Phase  6  ██  Particles + VFX             — GPU particles, VFX graph
Phase  7  ██  Camera System               — Cinemachine-like virtual cameras
Phase  8  █████  Editor                   — hierarchy, viewport, inspector, materials, terrain, splines
Phase  9  ██  UI / GUI Builder            — canvas, widgets, theming, layouts
Phase 10  ██  Scripting                   — Lua, hot-reload, visual scripting
Phase 11  ██  Debugger + Profiler         — script debug, CPU/GPU/memory profiler
Phase 12  ██  2D Tools                    — sprites, tilemaps, parallax, 2D physics
Phase 13  ██  Game Services               — IAP, ads, analytics, cloud, social, economy
Phase 14  ██  Export & Pipeline           — baking, packing, app manifest, build pipeline
Phase 15  ██  Multiplayer                 — netcode, lobby, relay, voice, anti-cheat
Phase 16  ██  Web + Cross-platform        — WASM, WebGPU, mobile, XR, consoles
Phase 17  ██  AA Polish & Scale           — job system, LOD, destruction, water, modding, ML
```

---

## Phase 0 — Developer Foundation
*The bedrock. Everything depends on this.*

### 0.1 Logging system (levels, channels, file, ring buffer)
`🔲 Not started` | `1-2 sprints`

A structured logging framework replacing raw `std::cout` / `std::cerr` scattered across the codebase. Log levels (trace/debug/info/warn/error/fatal) and channels (render, input, asset, physics, script…) allow fine-grained filtering. Multiple sinks output to the console, a rotating file, and an in-memory ring buffer that an ImGui console widget can consume live.

### 0.2 Config system (engine.ini, cvars, presets)
`🔲 Not started` | `1-2 sprints`

A key-value configuration store that reads from human-editable `.ini` files at startup and exposes every setting through typed console variables (cvars). Graphics presets (Low/Medium/High/Ultra) map to groups of cvars for one-click quality switching. All cvars are writable at runtime and persist to disk on exit.

### 0.3 ImGui integration + debug overlay (FPS, frame time, draw calls)
`🔲 Not started` | `1-2 sprints`

Dear ImGui wired into the SDL3 + OpenGL backend with input forwarding, docking, and a custom theme. Ships with a persistent stats overlay showing FPS, frame time breakdown (CPU/GPU), draw call count, triangle count, and memory usage. This is the foundation for all future editor, debug UI panels, and the developer console.

### 0.4 Console variables runtime (live tweak)
`🔲 Not started` | `1 sprint`

A developer console built on ImGui with autocomplete that lets you query and modify any cvar at runtime — `r.shadow_map_size 4096`, `s.master_volume 0.5`, `debug.draw_physics true`. Changes take effect immediately without recompilation. The console window is toggled with a hotkey (backtick) and maintains command history across sessions.

### 0.5 Assertions, crash handler (Breakpad)
`🔲 Not started` | `1 sprint`

A debug assertion system that catches invariant violations during development with file/line context, callstack printing, and optional breakpoint. In release builds, Google Breakpad or a lightweight equivalent captures native crash dumps, generates minidumps, and can upload them to a remote crash collection endpoint for analysis.

### 0.6 FileAccess abstraction (compressed, encrypted, memory)
`🔲 Not started` | `1 sprint`

A polymorphic file I/O layer that abstracts away the difference between reading from disk, from compressed archives, from encrypted blobs, or from in-memory buffers. All engine subsystems read through this single API, which means adding new storage backends (HTTP streaming, asset packs, encrypted DLC) requires no changes to consumer code.

---

## Phase 1 — Core Data Architecture
*The spine of any game.*

### 1.1 Tag / Layer system
`🔲 Not started` | `1 sprint`

Every entity can be assigned one or more tags ("Player", "Enemy", "Pickup") and belongs to a collision/render layer. Systems query by tag instead of iterating all entities, and physics filters collision pairs by layer mask. Tags are editable in the inspector and serialized with the scene.

### 1.2 Input action system (rebindable, gamepad + keyboard + touch)
`🔲 Not started` | `2-3 sprints`

Abstracts raw keycodes into semantic actions ("Jump", "Shoot", "MoveHorizontal") that can be rebound at runtime and persisted to a config file. Supports multiple input sources simultaneously — keyboard, mouse, gamepad (up to 8 controllers), touch, and pen. Each action can bind multiple triggers with a configurable deadzone and a priority system for UI vs gameplay contexts.

### 1.3 Scene serialization (YAML/binary, save/load)
`🔲 Not started` | `2 sprints`

The entire world — entities, components, transforms, prefab references — can be saved to and loaded from a file. A text-based format (YAML) is used during development for version control friendliness, while a binary format is available for faster loading in release builds. The system handles circular references, missing assets gracefully, and version migration for evolving schemas.

### 1.4 Prefab system (blueprints, inheritance, override, nested)
`🔲 Not started` | `2-3 sprints`

A reusable entity template stored as a standalone asset. When instantiated, the prefab creates a linked clone: changes to the prefab propagate to all instances, while individual instances can override specific properties. Supports nested prefabs (prefabs inside prefabs) and multi-level override hierarchies, matching the workflow of modern engine editors.

### 1.5 Scene management (additive, transitions, loading screen)
`🔲 Not started` | `2 sprints`

The engine can load multiple scenes additively and unload them independently, enabling streaming worlds, persistent UI layers, and split-screen. Built-in transitions (fade, cross-fade, loading screen with progress bar) handle the visual seam between scenes. Background loading with a dedicated worker thread prevents hitches.

### 1.6 Object pooling (template pool, recycle)
`🔲 Not started` | `1 sprint`

A generic pool container that pre-allocates a fixed number of entities or components and recycles them instead of allocating and freeing memory at runtime. Critical for bullet hells, particle spawners, and any system that creates and destroys objects frequently. The pool grows on demand if exhausted and can warm-populate at scene load time.

### 1.7 Messaging system (decoupled object→object)
`🔲 Not started` | `1 sprint`

A lightweight, type-safe message bus that lets entities and systems communicate without direct references. Any object can subscribe to a message type and any object can broadcast to all subscribers. Supports immediate dispatch, queued dispatch (deferred to end of frame), and optional filtering by sender or channel.

### 1.8 Resource system (import pipeline, baking, compression)
`🔲 Not started` | `2 sprints`

Extends the current asset manager with a formal import pipeline that converts source files (`.png`, `.gltf`, `.wav`) into engine-optimized runtime formats during an import step. Bakes textures into compressed GPU formats, pre-processes meshes for cache-friendly vertex layouts, and generates ready-to-load binary blobs. Imports are cached and only re-run when the source file changes.

### 1.9 Bundle splitting + Live Update (priority archives, streaming)
`🔲 Not started` | `2 sprints`

The game's content can be split into multiple `.pak` archives with configurable load priorities and mounting order. A "Live Update" system downloads optional bundles at runtime (HD textures, language packs, DLC) and hot-mounts them without restarting. Content integrity is verified through hashes, and bundles can be encrypted for distribution.

### 1.10 JSON / serialization utilities
`🔲 Not started` | `1 sprint`

A utility library for reading and writing JSON, BSON, and the engine's own binary serialization format. Provides reflection-like helpers to serialize POD structs and common engine types (Vec3, Mat4, Color, etc.) with a single macro or template call. Used by the save/load system, networking, and editor clipboard.

### 1.11 High-performance collections (NativeArray-like)
`🔲 Not started` | `1 sprint`

Drop-in replacements for `std::vector`, `std::unordered_map`, and similar containers that use a custom arena allocator and provide deterministic memory access patterns. These collections can be safely passed to the job system, support cache-line alignment, and avoid hidden allocations. They form the low-level foundation for the future ECS rewrite.

---

## Phase 2 — Audio
*50 % of immersion is sound.*

### 2.1 Audio engine (miniaudio / OpenAL Soft)
`🔲 Not started` | `2 sprints`

A cross-platform audio library integrated as the engine's sound server. Handles device enumeration, stream mixing, format decoding (WAV, Ogg, MP3), and resource management. The engine abstracted behind an `AudioServer` API so the backend can be swapped without touching game code.

### 2.2 3D spatial audio + doppler effect
`🔲 Not started` | `1 sprint`

Sounds positioned in 3D space attenuate with distance, pan according to their angle from the listener, and exhibit doppler pitch shift when either the source or listener moves. Uses HRTF-like filtering for directional perception and supports per-sound min/max distance curves.

### 2.3 Audio buses + effects (reverb, EQ, compressor, delay)
`🔲 Not started` | `2 sprints`

A bus routing graph inspired by audio workstations: each sound is sent to a bus, buses can feed other buses, and each bus hosts a chain of DSP effects. Includes built-in effects — reverb (convolution and algorithmic), parametric EQ, compressor/limiter, delay/echo, chorus, flange, and distortion. The entire bus layout can be saved as an asset.

### 2.4 Streaming (Ogg, MP3, WAV, tracker modules)
`🔲 Not started` | `1 sprint`

Large audio files (music, ambient beds, dialogue) are streamed from disk in small chunks rather than loaded entirely into memory. Supports seeking, looping with configurable loop points, and gapless playback between tracks. Decoding runs on a worker thread so streaming never stalls the audio callback.

### 2.5 Listener (attached to camera or other entity)
`🔲 Not started` | `1 sprint`

The audio listener is an entity component that defines where the "ear" of the player is in the world. By default it follows the active camera, but can be attached to any entity for split-screen, security cameras, or audio debugging. Multiple listeners are supported for local multiplayer.

### 2.6 Procedural audio / synthesizer
`🔲 Not started` | `1 sprint`

A simple synthesis engine that generates waveforms (sine, square, saw, noise, FM) at runtime. Useful for UI sounds, footsteps, weapon hits, and dynamic music systems where pre-recorded samples would be too static. Parameters (frequency, amplitude, envelope) are controllable through script at audio-rate resolution.

### 2.7 Voice chat (Vivox-like)
`🔲 Not started` | `2 sprints`

Peer-to-peer and client-server voice communication built into the engine. Handles microphone capture, encoding/decoding, push-to-talk, volume control per speaker, and mute lists. The voice data travels over the same transport layer as the game's networking for simplified NAT traversal.

### 2.8 Mixer / ducking / sidechain
`🔲 Not started` | `1 sprint`

A master mixer that allows per-bus volume, solo, and mute controls with real-time automation. Sidechain compression (audio ducking) automatically lowers background music when dialogue plays or lowers SFX when the player speaks in voice chat, all configured through a simple declarative API.

---

## Phase 3 — Physics + Navigation
*Make the world feel solid.*

### 3.1 Physics engine (Jolt Physics)
`🔲 Not started` | `2-3 sprints`

Jolt Physics integrated as the engine's rigid-body dynamics system. Chosen for its production pedigree (used in Horizon Forbidden West, Red Dead Redemption), deterministic simulation, excellent multithreading support, and permissive license. Provides a C++ API wrapped behind an engine-abstracted `PhysicsServer` interface.

### 3.2 Collision shapes (box, sphere, capsule, cylinder, mesh, convex)
`🔲 Not started` | `1 sprint`

A library of primitive collision shapes that can be attached to entities: box, sphere, capsule, cylinder, convex hull (computed from a mesh), and triangle mesh (for static environment geometry). Shapes are editable in the inspector, visualized with wireframe overlays, and can be compound (multiple shapes per body).

### 3.3 Physics bodies (rigid, static, character, vehicle)
`🔲 Not started` | `2 sprints`

Four body types mapped to entity components: `RigidBody` (full dynamics, affected by forces), `StaticBody` (immovable, for level geometry), `CharacterBody` (kinematic with collision response and step-up, for player/ NPCs), and `VehicleBody` (arcade vehicle physics with wheel colliders, suspension, and engine simulation).

### 3.4 Soft body + Cloth simulation
`🔲 Not started` | `1-2 sprints`

Deformable bodies — flags, capes, jelly, trampolines, destructible objects — simulated with constraint-based soft bodies. Cloth is a specialized soft body with anisotropic stretching, bending resistance, wind response, and collision against both static and dynamic objects. Both run on a dedicated solver island separate from the main rigid-body simulation.

### 3.5 Area / trigger zones
`🔲 Not started` | `1 sprint`

Invisible volumetric zones that detect when physics bodies enter, stay inside, or exit. Used for script triggers (spawn enemies when player walks through a door), damage zones (lava), and environmental effects (underwater reverb). Areas can overlap and each one fires typed events into the messaging system.

### 3.6 Joints (pin, hinge, slider, cone, 6DOF)
`🔲 Not started` | `1 sprint`

Constraints that connect two bodies or a body to a fixed point in space. Pin joint (ball-and-socket), hinge joint (door), slider joint (piston), cone joint (shoulder-like angular limit), and 6DOF joint (fully configurable per-axis limits and motors). Joints can break under excessive force and drive their targets through spring-damper motors.

### 3.7 Raycast, shapecast, overlap queries
`🔲 Not started` | `1 sprint`

Synchronous and asynchronous query functions against the physics world. Raycasts return the first hit or all hits along a line. Shapecasts sweep a collision shape through space and return all contacts. Overlap tests detect all shapes overlapping a volume without movement. All queries support layer masks and optional ignore lists.

### 3.8 Navigation mesh (Recast/Detour)
`🔲 Not started` | `2 sprints`

Recast generates a navigation mesh from the level geometry (static triangle meshes) with configurable agent radius, height, max slope, and step height. Detour provides pathfinding queries on the generated navmesh. The navmesh is automatically updated when level geometry changes and can be baked at runtime for procedurally generated levels.

### 3.9 A* pathfinding + dynamic obstacle avoidance
`🔲 Not started` | `1-2 sprints`

A* search on the navigation mesh returns a path from point A to B, optimized for path length and smoothed through corner cutting and string-pulling. Dynamic obstacle avoidance lets agents (NPCs, players in multiplayer) steer around each other at runtime using RVO (Reciprocal Velocity Obstacles) integrated with the navmesh corridor.

### 3.10 Wind zones (affects cloth, vegetation, particles)
`🔲 Not started` | `1 sprint`

Spatial regions that apply a directional force to any physics object tagged as "wind-sensitive." Animates cloth, vegetation (grass, trees), particle systems, and soft bodies with a configurable turbulence, gust frequency, and force magnitude. Multiple wind zones can overlap and blend their contributions.

### 3.11 Terrain physics (heightmap collision)
`🔲 Not started` | `1 sprint`

A specialized collision shape that reads from the terrain's heightmap at runtime, providing efficient collision against large outdoor landscapes without generating a triangle mesh. Handles wheel-terrain contact for vehicles, footstep detection, and proper friction coefficients per-terrain-texture zone.

### 3.12 Ragdoll system
`🔲 Not started` | `1-2 sprints`

When a character dies or is hit by a large impulse, the skeleton transitions from animation-driven poses to a physics-driven ragdoll. Joints are automatically created from the skeleton hierarchy, and each bone becomes a rigid body with appropriate mass and collision shape. Blends from animation to ragdoll over a configurable time window.

---

## Phase 4 — Animation
*Bring characters to life.*

### 4.1 Skeletal animation (glTF skinning)
`🔲 Not started` | `2-3 sprints`

Loads skinned meshes and animation clips from glTF files and drives the skeleton in real-time. Each vertex is deformed by up to 4 weighted joint transforms (linear blend skinning). The animation system works with the existing `MeshRenderer` component and supports multiple simultaneous animation clips blended together.

### 4.2 Animation state machine + blend tree
`🔲 Not started` | `2 sprints`

A visual state machine for selecting and transitioning between animation clips based on parameters (speed, direction, jumping, health). States can contain blend trees that cross-fade between multiple clips (e.g., blend between walk and run based on speed). Transitions have configurable duration, sync offsets, and conditions.

### 4.3 Morph targets / blend shapes
`🔲 Not started` | `1-2 sprints`

Per-vertex deformations driven by weight values, loaded from glTF morph targets. Used for facial expressions, talking, and fine-grained deformation that skeletal animation cannot capture. Multiple morph targets blend additively and can be combined with skeletal animation. Runtime weight control through script enables dynamic lip-sync and emotion systems.

### 4.4 Inverse kinematics (two-bone + CCD)
`🔲 Not started` | `1-2 sprints`

Solves the end-effector position of a bone chain to reach a target location while respecting joint angle limits. Two-bone IK (arm/leg reaching) is solved analytically for speed. CCD (Cyclic Coordinate Descent) handles arbitrary-length chains (tentacles, tails, spider legs). Both modes support pole vectors for elbow/knee direction hints.

### 4.5 Animation Rigging (constraints, aim, look-at)
`🔲 Not started` | `2 sprints`

A post-process animation layer that applies procedural rigging constraints on top of the animation output. Includes aim constraint (head/weapon aims at a target), look-at constraint (eyes follow an object), parent constraint (hand attaches to a moving platform), and chain constraints for FK/IK blending. Fully scriptable for custom rigging behaviours.

### 4.6 Animation tracks (properties, methods, audio, bezier)
`🔲 Not started` | `1-2 sprints`

Animation clips can drive any component property (transform, light color, material parameter, UI position) over time using keyframes with configurable interpolation curves. Special track types call script methods at precise timestamps (trigger a sound at frame 42), play audio clips, and animate Bezier-shaped paths for camera movement.

### 4.7 Root motion
`🔲 Not started` | `1 sprint`

The character's movement is driven by the animation itself rather than scripted velocity. When a "run" animation displaces the root bone forward, the character's transform follows. Supports in-place animations (no root motion) blended with root-motion animations and configurable motion multipliers per clip.

### 4.8 Animation retargeting (humanoid)
`🔲 Not started` | `2 sprints`

Animation clips created for one humanoid skeleton can be retargeted to any other humanoid skeleton, regardless of bone proportions or rest pose. The system maps bones through a standardized humanoid rig profile (hips, spine, head, arms, legs, fingers). This lets you buy animation packs from any source and use them on any character without re-exporting.

### 4.9 Spine / Rive integration (2D skeletal)
`🔲 Not started` | `1-2 sprints`

First-class support for Spine 2D skeletal animations and Rive state machines. Spine provides bone-based 2D animation with meshes, weights, and attachments. Rive adds interactive state machines with input-driven animation transitions. Both render as 2D sprites in the 3D scene or in a dedicated 2D layer, and their controls are accessible through scripting.

### 4.10 Alembic import (film-grade animation)
`🔲 Not started` | `1 sprint`

Alembic is a format used by film and VFX pipelines for caching baked animation and geometry caches. Supports importing Alembic `.abc` files as baked animation clips and point-cache geometry sequences. This allows using high-end animation from Maya, Houdini, or Blender without re-rigging for the game engine skeleton.

---

## Phase 5 — Rendering Advanced
*Visual quality is made here.*

### 5.1 Shadow mapping (CSM 2/4 splits, PCSS)
`🔲 Not started` | `2-3 sprints`

Directional light shadows using Cascaded Shadow Maps with 2 or 4 splits, distributing shadow resolution across distance from the camera. Omni lights use cubemap or dual-paraboloid shadow maps. Spot lights use a single shadow map. PCSS (Percentage-Closer Soft Shadows) varies penumbra width based on light size and distance, producing realistic soft shadows.

### 5.2 HDR + tonemapping (ACES, Filmic, AgX, Reinhard)
`🔲 Not started` | `1-2 sprints`

Rendering in high dynamic range (FP16 or FP10 render targets) preserves brightness information before final output. A tonemapping operator maps the infinite HDR range to the display's limited range. Ships with ACES, Filmic (Unreal-derived), AgX (Blender's filmic), and Reinhard operators, switchable at runtime.

### 5.3 Bloom / glow (with lens dirt)
`🔲 Not started` | `1-2 sprints`

Bright areas of the frame bleed into surrounding pixels, simulating the physical response of camera lenses. Implements a multi-scale Gaussian pyramid for efficient blur at varying kernel sizes. Supports an optional lens dirt texture that adds colored artifacts around bright sources, and several blend modes (screen, soft light, add, replace).

### 5.4 SSAO / SSIL (screen-space ambient occlusion / indirect lighting)
`🔲 Not started` | `2 sprints`

SSAO estimates ambient occlusion from the depth buffer, darkening creases, corners, and contact points between objects. SSIL goes further by estimating one-bounce indirect lighting from the colour buffer, adding subtle colour bleeding. Both run at half or full resolution with an optional bilateral blur for noise reduction.

### 5.5 Depth of field (bokeh near/far)
`🔲 Not started` | `1-2 sprints`

Simulates a camera lens with finite aperture, blurring objects outside the focal plane. Supports near and far blur with adjustable focal distance and aperture size. The bokeh shape can be a hexagon, circle, or custom 6-tap pattern. Uses a gather-based approach with a half-resolution coc (circle of confusion) buffer.

### 5.6 SSR (screen-space reflections)
`🔲 Not started` | `1-2 sprints`

Reflects the visible scene onto shiny surfaces using the colour and depth buffers as a reflection source. Traces rays in screen space using hierarchical depth searching, then fades out where screen-space data is missing (at screen edges or behind the camera). Roughness blurs the reflection according to the material's roughness value.

### 5.7 TAA / FXAA / MSAA
`🔲 Not started` | `1-2 sprints`

Three anti-aliasing techniques covering different quality/performance trade-offs. TAA (Temporal Anti-Aliasing) jitters the camera each frame and accumulates history for high-quality sub-pixel detail at minimal cost. FXAA is a fast post-process pass that smooths edges. MSAA multi-samples the rasterizer (2x, 4x, 8x) for clean geometric edges at higher cost.

### 5.8 Resolution scaling + FSR2 (AMD upscaling)
`🔲 Not started` | `1-2 sprints`

Renders the 3D scene at a lower resolution than the display then upscales to native. Bilinear and FSR1 (sharpening) are the low-cost options. FSR2.2 provides temporally stable, high-quality reconstruction with an optional sharpening pass. Texture mipmap LOD bias is automatically adjusted to maintain detail at lower render scales.

### 5.9 Skybox (panorama, procedural PBR, custom shader)
`🔲 Not started` | `1 sprint`

Three sky modes rendered as a full-screen background. Panorama renders an HDR cubemap (from an equirectangular texture). Procedural sky generates sky colour, sun position, and clouds from physical parameters (turbidity, ozone, ground albedo). Custom sky shaders let artists write GLSL that runs per-fragment on the sky dome.

### 5.10 Fog (depth, height, volumetric)
`🔲 Not started` | `2 sprints`

Exponential depth fog and height-based fog with configurable density, colour, and sky-blending. Volumetric fog uses a 3D froxel grid raymarched in screen space, reacting to light and shadow cast by directional, omni, and spot lights. Fog volumes with box, sphere, and custom density-texture shapes add localized fog areas.

### 5.11 Decals
`🔲 Not started` | `1 sprint`

Projected textures that overlay on top of existing geometry without modifying the mesh. Supports albedo, normal, ORM (occlusion/roughness/metallic), and emissive decals. Normal fade prevents decals from stretching on steep angles. Uses clustered rendering with the same visibility system as lights, so thousands of decals are performant.

### 5.12 GI (lightmaps GPU, VoxelGI, SDFGI)
`🔲 Not started` | `3-4 sprints`

Three global illumination approaches for different scenarios. GPU lightmaps bake indirect lighting into per-texel lightmap UVs using a compute-shader path tracer (fast, static). VoxelGI injects dynamic lights into a voxel grid and traces cones for diffuse and rough specular bounces. SDFGI uses a signed-distance field of the scene for infinite-bounce GI in large open worlds.

### 5.13 Light probes + reflection probes
`🔲 Not started` | `1-2 sprints`

Light probes capture spherical harmonic coefficients at points in space and interpolate between them to light dynamic objects with baked indirect light. Reflection probes capture cubemap snapshots of the scene from specific positions, used for glossy reflections on moving objects. Both support runtime blending and culling.

### 5.14 Compute shaders (post-process, simulation)
`🔲 Not started` | `2 sprints`

OpenGL compute shader support enabling general-purpose GPU programming beyond the rasterization pipeline. Used for post-processing effects (bloom, SSAO, tonemapping), particle simulation, texture processing, and physics calculations. Dispatches are integrated with the render graph to synchronize properly with the graphics pipeline.

### 5.15 Texture compression (BCn, Basis Universal, ETC2, ASTC)
`🔲 Not started` | `1-2 sprints`

All textures are compressed at import time to a GPU-friendly format. BC1-7 (desktop), ETC2 (mobile, fallback), ASTC (mobile, high quality), and Basis Universal (web, cross-platform) are all supported. The import pipeline selects the best format for the target platform automatically, and mipmaps are generated with proper filter kernels.

### 5.16 GPU denoising (lightmaps, ray tracing)
`🔲 Not started` | `1 sprint`

A GPU-based denoising filter that cleans up noisy images generated by path-traced lightmaps, real-time ray tracing, or Monte Carlo GI. Implements a spatiotemporal variance-guided filter (SVGF) that reuses history across frames and edge-stops on depth and normal discontinuities. Configurable quality/performance slider.

### 5.17 Variable rate shading (VRS)
`🔲 Not started` | `1 sprint`

Allows shading rate to vary across the frame: shaded at full rate in detailed regions, half or quarter rate in uniform areas. Used to improve performance by reducing shading cost in shadowed, out-of-focus, or motion-blurred regions. Requires tier-2 VRS hardware support (NVIDIA Turing+, AMD RDNA2+).

### 5.18 DLSS / XeSS upscaling (vendor-specific)
`🔲 Not started` | `2 sprints`

Vendor-specific deep-learning upscaling technologies. NVIDIA DLSS uses AI reconstruction from a lower-resolution input with temporal feedback. Intel XeSS is an open-standard alternative with similar quality on Intel, NVIDIA, and AMD hardware. Both are integrated as optional render-mode overrides, falling back to FSR2 if hardware support is missing.

### 5.19 Shader Graph (visual shader editor)
`🔲 Not started` | `3-4 sprints`

A node-based shader editing tool in the engine editor, allowing artists to create vertex, fragment, and compute shaders without writing GLSL. The graph compiles to optimized GLSL with automatic texture binding and uniform reflection. Ships with a library of utility nodes (math, noise, PBR, UV, lighting) extendable through custom node plugins.

### 5.20 Ray tracing (shadows, reflections, AO)
`🔲 Not started` | `3-4 sprints`

Real-time ray tracing using Vulkan ray tracing extensions or OpenGL ray query (via NV/AMD extensions). Ray-traced shadows replace CSMs with pixel-perfect hard and soft shadows. Ray-traced reflections replace SSR with correct reflections for off-screen objects. Ray-traced ambient occlusion provides ground-truth contact shadows. Falls back to raster techniques on non-RT hardware.

---

## Phase 6 — Particles + VFX

### 6.1 GPU particles (2D + 3D)
`🔲 Not started` | `2 sprints`

Particle simulation running entirely on the GPU via compute shaders, supporting millions of particles with zero CPU overhead for simulation. Each particle has position, velocity, colour, size, rotation, and lifetime — all updated in GPU buffers. Emitted from a configurable shape (point, sphere, box, cone, mesh surface).

### 6.2 CPU particles
`🔲 Not started` | `1 sprint`

A fallback particle system running on the CPU for platforms without compute shader support and for effects with complex per-particle logic that is easier to express in C++ or script. Shares the same emitter, renderer, and configuration API as GPU particles so effects can be authored once and switch backends based on platform capability.

### 6.3 Emitters, subemitters, attractors, collision
`🔲 Not started` | `2 sprints`

Particle emitters spawn particles with configurable rate, burst count, and initial conditions (position, velocity, colour spread). Subemitters spawn a secondary particle effect when a primary particle dies, collides, or reaches a specific age (e.g., a spark that spawns a smoke puff on death). Attractors pull particles toward points or shapes. Collision detection against the physics world lets particles bounce, stick, or die on impact.

### 6.4 Trails (ribbon, tube)
`🔲 Not started` | `1 sprint`

Trailing geometry that follows moving particles or any moving entity. Ribbon trails are flat strips with per-vertex colour and UV that fade along the trail length. Tube trails are cylindrical geometry inheriting the source's orientation, suitable for lightning, grappling hooks, and laser beams. Both modes support variable width and colour over the trail lifetime.

### 6.5 Curve editor for properties over lifetime
`🔲 Not started` | `1 sprint`

Each particle property (size, colour, velocity, rotation, alpha) is controlled by a curve that maps particle age to value. The curve editor provides a spline-based UI with keyframe points, configurable interpolation (linear, smooth, stepped, bezier), and random variation. Curves can be saved and reused across multiple particle effects.

### 6.6 Billboarding + camera-facing sprites
`🔲 Not started` | `1 sprint`

Particle quads automatically rotate to face the camera, ensuring sprites always appear as 2D images regardless of viewing angle. Supports three billboard modes: standard (face camera), axis-aligned (rotate only around Y, for smoke/clouds), and world-oriented (no billboarding, for confetti-like effects). Animated atlases allow frame-based animation within each particle.

### 6.7 Custom particle shaders
`🔲 Not started` | `1 sprint`

Artists can write custom GLSL shaders that run on each particle, controlling vertex position, colour, and UV per-particle. The shader receives per-particle data (lifetime, random seed, velocity) as vertex attributes and can sample textures, compute noise, and modify appearance arbitrarily. Enables effects like electricity, birds flocking, and magical glows.

### 6.8 Visual Effect Graph (node-based VFX authoring)
`🔲 Not started` | `3-4 sprints`

A high-level node-based effect authoring tool inspired by Unity's Visual Effect Graph. Artists compose effects by connecting nodes — spawners, initializers, updates, outputs — without writing code or shaders. The graph is compiled to an efficient compute-shader-based simulation at runtime. Includes GPU event system for complex effect chains (death → explosion → debris → smoke).

---

## Phase 7 — Camera System

### 7.1 Virtual Camera (Cinemachine-like)
`🔲 Not started` | `2-3 sprints`

A virtual camera component that defines a desired view (position, rotation, FOV, lens properties) independently of the actual rendering camera. Multiple virtual cameras coexist in the scene, and the system blends between them based on priority, triggers, or timeline control. Decouples gameplay camera logic from rendering.

### 7.2 Camera blending (cut, fade, smooth, timeline-driven)
`🔲 Not started` | `1 sprint`

When control transfers from one virtual camera to another, the transition can be a hard cut, a fade-to-black, or a smooth positional/rotational blend with configurable damping. Timeline-driven blends are authored on a cinematic timeline with keyframed camera weights for complex multi-camera sequences.

### 7.3 Follow / look-at / framing targets
`🔲 Not started` | `1-2 sprints`

Virtual cameras can follow a target entity (player, vehicle, object) with configurable damping, offset, and look-ahead. The look-at mode keeps a target centred in the frame while the camera orbits. Framing mode adjusts the camera distance to keep a group of targets within the screen bounds — essential for split-screen and co-op.

### 7.4 Camera noise / shake / impulse
`🔲 Not started` | `1 sprint`

Adds procedural noise to the camera transform for screen shake, hit impacts, and handheld realism. Noise is defined as a waveform (perlin, sine, random) with configurable frequency, amplitude, and decay over time. Multiple noise sources stack additively, so an explosion can trigger a short high-amplitude shake while walking applies subtle constant noise.

### 7.5 Collision avoidance (camera push-through walls)
`🔲 Not started` | `1 sprint`

When a third-person camera would clip through geometry, it automatically pushes forward to avoid the occlusion. A sphere cast from the target toward the desired camera position finds the nearest obstruction and repositions the camera in front of it. The transition is smoothed to avoid jarring snaps, and the system can optionally show a transparent silhouette of the occluding wall.

---

## Phase 8 — Editor
*The most important tool. This is where games are actually made.*

### 8.1 Scene tree / hierarchy panel
`🔲 Not started` | `2-3 sprints`

A tree view of all entities in the current scene, showing parent-child relationships, enabled/disabled state, and entity names. Supports drag-and-drop reparenting, multi-select, right-click context menus (create, duplicate, delete, copy/paste), search/filter by name or component type, and a "ping in scene" action that selects and focuses the entity.

### 8.2 Viewport 3D + gizmos (translate/rotate/scale)
`🔲 Not started` | `3-4 sprints`

A render-to-texture viewport that shows the scene from the editor camera, fully interactive. Viewport controls include orbit (alt+click), pan (middle-mouse), dolly (scroll), and focus-selected (F key). Transform gizmos (translate arrow, rotate arcball, scale cube) let you manipulate entities directly in the 3D view with snapping (grid, angle, surface) and pivot/global coordinate toggle.

### 8.3 Inspector panel (all components editable)
`🔲 Not started` | `2-3 sprints`

A property grid that displays all components of the selected entity and lets you edit their values in real-time. Each component type provides a custom inspector UI (transform shows position/rotation/scale fields, camera shows FOV/clip-planes, etc.). Supports vector sliders (click-drag on labels), colour pickers, asset drag-drop, and add/remove component buttons.

### 8.4 Asset browser (thumbnails, drag-drop, search)
`🔲 Not started` | `2 sprints`

A file system panel that browses the project's asset directory with thumbnail previews for supported types (textures, materials, models, scenes, scripts). Supports filtering by type, search by name and metadata tags, and drag-drop from the browser into the scene (instantiates a mesh) or into the inspector (assigns a material/texture). Displays metadata columns: size, last modified, import status.

### 8.5 Material editor (Phong + PBR, live preview)
`🔲 Not started` | `2-3 sprints`

A graphical material editing panel showing a real-time preview of a sphere or character model under studio lighting. Artists tweak PBR parameters (albedo, metallic, roughness, normal, AO, emissive) and Phong parameters (ambient, diffuse, specular, shininess) with sliders and colour pickers. Texture slots support drag-drop from the asset browser. Changes are saved to the material asset immediately.

### 8.6 Prefab editor (create / apply / revert override)
`🔲 Not started` | `2-3 sprints`

A dedicated editing mode where you open a prefab asset in isolation, editing its entity hierarchy and component defaults without affecting scene instances. Supports "apply" to push changes from a scene instance back to the prefab, "revert" to discard a specific override, and "select" to navigate to overridden properties with visual highlighting (blue = override, bold = new value).

### 8.7 Animation editor (timeline, keyframes, curves)
`🔲 Not started` | `3-4 sprints`

A keyframe animation authoring tool with a timeline track view. Each animated property is a track with keyframes that can be added, moved, deleted, and snapped to frames or seconds. The curve editor shows the interpolation between keyframes as a spline with adjustable tangents (auto, linear, stepped, custom bezier). Supports animation events (calling script functions at specific times) and animation preview directly on the selected entity in the viewport.

### 8.8 Terrain editor (heightmap, splatmap, vegetation)
`🔲 Not started` | `2-3 sprints`

A set of tools for sculpting terrain heightmaps with brushes (raise, lower, smooth, flatten, noise) with adjustable radius and intensity. Splatmap painting blends up to 4 texture layers per-pixel (grass, rock, sand, snow). Vegetation painting places instanced grass, bushes, and trees that respond to wind zones and camera distance LOD. All operations support undo/redo.

### 8.9 Particle editor (live preview)
`🔲 Not started` | `1-2 sprints`

A visual particle effect editor that plays the effect in a loop inside a dedicated preview panel. Every emitter property (rate, lifetime, speed, size, colour, shape) is exposed as a slider or colour picker with immediate feedback. The curve editor for lifetime properties is integrated into the same panel. Multiple emitters within one effect are shown as a list with individual enable/disable toggles.

### 8.10 GUI editor (canvas, widgets, layouts)
`🔲 Not started` | `2-3 sprints`

A what-you-see-is-what-you-get editor for the UI system. A canvas represents the screen or a viewport, and widgets (buttons, labels, images, panels, sliders) are placed and sized visually. Layout containers (HBox, VBox, Grid) are shown with their distribution controls. The editor previews the current anchor configuration, responsive behaviour for different screen sizes, and the current theme.

### 8.11 Undo/Redo (command pattern)
`🔲 Not started` | `1-2 sprints`

A generic undo/redo system based on the command pattern, where every user action in the editor creates a command object that knows how to execute and undo itself. Commands are stored in an unlimited history stack (configurable max size) and grouped by "transaction" boundaries (drag start→end, a single property edit). The scene hierarchy, inspector, and viewport share the same undo stack.

### 8.12 Play mode + hot-reload in-editor
`🔲 Not started` | `2 sprints`

A "Play" button that runs the current scene inside the editor viewport, simulating the game as if it were a standalone build. Scripts run, physics simulates, and audio plays. While in play mode, changes to scripts and assets are hot-reloaded without stopping the game. Exiting play mode restores the scene to its pre-play state. Supports "Pause" and "Frame Step" while in play mode.

### 8.13 Plugin system (editor scripting API)
`🔲 Not started` | `2-3 sprints`

The editor itself can be extended through plugins written in Lua (and later visual scripting). Plugins can add custom panels, menu items, toolbar buttons, importers, exporters, gizmos, and inspector customizations. A documented API exposes the editor's internal services: scene manipulation, asset management, undo/redo, and UI toolkit. Plugins are loaded from the project's `plugins/` directory.

### 8.14 Theme editor
`🔲 Not started` | `1-2 sprints`

A tool for customizing the editor's visual appearance. Users can edit colours, fonts, icon sizes, spacing, and panel layout presets. Themes are stored as JSON files and shareable between projects. A live preview panel shows the current theme applied to all editor UI elements. The default theme emulates a dark IDE-like appearance with configurable accent colour.

### 8.15 Asset refactoring (rename propagation)
`🔲 Not started` | `1 sprint`

Renaming an asset file in the project browser automatically updates all references to it across the entire project — in scenes, prefabs, materials, scripts, and configuration files. A refactoring preview dialog shows all affected files before applying the change. If a rename cannot be fully resolved, the system flags broken references for manual attention.

### 8.16 ProBuilder (in-editor 3D modeling)
`🔲 Not started` | `3-4 sprints`

A suite of in-editor modeling tools for creating and editing 3D geometry without an external DCC tool. Create primitives (box, sphere, cylinder, plane, stair, torus) and edit their geometry through vertex/edge/face extrusion, inset, bevel, loop cut, and subdivision. Ideal for level prototyping, architectural visualization, and creating simple props. Models can be exported to glTF or saved as native mesh assets.

### 8.17 Polybrush (vertex painting, sculpting)
`🔲 Not started` | `2-3 sprints`

Vertex painting, sculpting, and texture blending tools that work directly on mesh geometry. Sculpt brushes (inflate, pinch, smooth, flatten) deform vertex positions. Paint brushes assign colours and blend weights per vertex for texture layer compositing. A brush preview shows the radius and falloff in the viewport. Results are saved as modified mesh assets without destructive overwrite.

### 8.18 Splines editor (roads, rails, paths)
`🔲 Not started` | `1-2 sprints`

A spline tool for creating curved paths in the scene. Control points with editable tangent handles define Catmull-Rom, Bezier, or cubic splines. The spline can be extruded into geometry (roads, pipes, rails), used as a camera animation path, or followed by entities (roller-coaster, train, patrol path). The editor shows the spline in the viewport and provides a "spline follower" component.

### 8.19 Device Simulator (mobile preview)
`🔲 Not started` | `1-2 sprints`

Resizes the editor viewport to match the resolution, aspect ratio, notch size, and safe-area of popular mobile devices (iPhone, iPad, Android phones, tablets). Touch input is simulated through mouse click-drag and multi-touch via keyboard modifiers. The simulator helps test responsive UI layouts, touch controls, and performance characteristics for mobile targets without deploying to a device.

### 8.20 Tutorial authoring framework
`🔲 Not started` | `1 sprint`

A system for creating interactive tutorials inside the editor that guide users through engine features. Tutorials are authored as JSON files with steps that highlight specific UI elements, display instructions, prompt for actions (select an entity, create a material, run the game), and wait for those actions to complete before advancing. Useful for onboarding new team members and creating in-engine documentation.

### 8.21 Online class reference + offline docs
`🔲 Not started` | `1-2 sprints`

An integrated documentation browser that displays the engine's API reference, class hierarchy, and usage examples. Fetches the latest docs from an online source but caches them for offline use. Search across all classes, functions, properties, and signals with fuzzy matching. Context-sensitive help (F1 on a selected item in the inspector or script editor) jumps directly to the relevant page.

---

## Phase 9 — UI / GUI Builder

### 9.1 Canvas + widget system (buttons, labels, sliders, inputs)
`🔲 Not started` | `2-3 sprints`

A retained-mode UI system built around a hierarchy of `Control` nodes on a render canvas. Ships with standard widgets: Button (clickable with pressed/hover/disabled states), Label (text with font, colour, alignment), Slider (horizontal/vertical, range, step), TextInput (single and multi-line, selection, clipboard), Image (sprite from asset), Panel (coloured or textured rectangle), and ProgressBar.

### 9.2 Containers (HBox, VBox, Grid, Flow, Margin, Scroll)
`🔲 Not started` | `1-2 sprints`

Automatic layout containers that position and size their children according to rules. HBox and VBox arrange children horizontally or vertically with spacing and alignment. Grid arranges in a fixed-column grid. Flow wraps children like text (responsive). Margin adds a border. CenterContainer centers children. ScrollContainer adds scrollbars to oversized content. Containers can be nested arbitrarily.

### 9.3 Anchors + responsive layouts (portrait/landscape)
`🔲 Not started` | `1-2 sprints`

Each control node has anchors (left, right, top, bottom) expressed as fractions of the parent canvas. Anchor presets (top-left, top-right, bottom, centre, stretch, full-rect) enable common layouts with a single click. The system supports dynamic aspect-ratio changes (device rotation, window resize) by recomputing positions and sizes based on anchor rules.

### 9.4 Theming (StyleBox flat/texture, rounded corners, shadows)
`🔲 Not opened` | `2 sprints`

A declarative theming system that separates widget appearance from widget logic. Themes define colours, fonts, icons, and StyleBoxes for each widget state (normal, hover, pressed, disabled, focused). StyleBoxFlat draws procedurally with support for rounded corners (per-corner radius), drop shadows, per-border widths, and antialiased edges. StyleBoxTexture uses a 9-sliced texture with configurable patch sizes.

### 9.5 Rich text / BBCode (coloured, animated, hyperlinks)
`🔲 Not started` | `1-2 sprints`

A text widget that renders BBCode-formatted strings with support for colour tags, bold/italic/underline, font size changes, image inlining, hyperlinks (clickable with URL dispatch), and custom effects like rainbow, wave, shake, and fade. Effects are animated per-character using CPU-driven vertex manipulation. The widget auto-sizes and supports scrolling for overflow content.

### 9.6 9-slice texturing + SDF fonts (TextMeshPro-like)
`🔲 Not started` | `1-2 sprints`

9-slice scaling divides a texture into 9 regions (4 corners, 4 edges, 1 centre) so that panels and buttons scale without distorting the border detail. SDF (signed-distance field) font rendering pre-computes per-glyph distance fields for resolution-independent text that stays sharp at any size, supports bold/italic/outline/shadow without separate font files, and enables glow and softness effects.

### 9.7 Tree / table view
`🔲 Not started` | `1 sprint`

A hierarchical tree widget with expandable/collapsible items, icons, custom cell rendering, column headers (resizable, sortable), and multi-selection. Table mode gives all items the same depth with sort-by-column. Used for the scene hierarchy, asset browser, profiler data display, and any data-grid needs.

### 9.8 Colour picker (RGB, HSV, HEX)
`🔲 Not started` | `1 sprint`

A colour-picking widget with RGB and HSV modes, a hue-saturation field, a lightness/alpha slider, and a HEX text input. Supports copy/paste of colour values, a colour history (recently used), eyedropper tool (sample colour from any pixel on screen), and an alpha channel toggle. The picker is embedded in the inspector for colour properties.

### 9.9 Popup / dropdown / context menu / tooltip
`🔲 Not started` | `1 sprint`

Transient overlay UI elements positioned relative to a parent or cursor. PopupMenu displays a vertical list of items with icons, shortcuts, submenus, separators, and radio/check items. Dropdown (OptionButton) shows the selected value and opens a popup on click. ContextMenu appears on right-click with contextual items. Tooltip appears on hover, supports multi-line rich text, and auto-positions to stay on screen.

### 9.10 Multi-window support
`🔲 Not started` | `1 sprint`

The engine and editor can open multiple OS windows within a single process. Each window has its own canvas and event loop, useful for multi-monitor editor layouts (scene view on one monitor, inspector on another), in-game secondary windows (inventory, map), and spectator viewports. Windows can be decorated, borderless, always-on-top, or transparent with click-through.

---

## Phase 10 — Scripting
*Empower designers to build game logic.*

### 10.1 Scripting language (Lua via sol3)
`🔲 Not started` | `2-3 sprints`

Lua 5.4 integrated through the sol3 library, providing a seamless C++ API binding layer. Engine types (Vec3, Entity, Transform, Component, Material) are bound to Lua with automatic type checking and documentation. Lua scripts attach to entities as script components and receive lifecycle callbacks (on_init, on_update, on_destroy, on_collision, on_trigger).

### 10.2 Script hot-reload
`🔲 Not started` | `1 sprint`

Changes to a Lua script file are detected by the file watcher and reloaded into the running game without restarting. The entity's state (variables, references) is preserved across reload by storing it in a table separate from the script's function definitions. Reload happens within a single frame, and errors during reload are caught and reported with the line number.

### 10.3 Script editor (syntax highlighting, autocomplete, linting)
`🔲 Not started` | `2-3 sprints`

A built-in code editor with syntax highlighting for Lua, GLSL, YAML, JSON, and the engine's native scripting language. Features include line numbers, code folding, bracket matching, word wrap, multi-cursor editing, find/replace with regex, and go-to-line. Autocomplete shows engine API symbols, script-local variables, and function signatures. A linter runs on file save and reports errors/warnings in a panel.

### 10.4 Signals / event system
`🔲 Not started` | `1 sprint`

A signal-slot pattern where entities and components emit typed signals that other scripts can connect to. Signals are defined declaratively (in script or in C++) and carry payloads (e.g., `on_hit(damage: int, source: Entity)`). The messaging bus and signals share the same underlying dispatch mechanism, enabling decoupled communication across the entire game.

### 10.5 Cross-language (C++ engine + Lua scripts)
`🔲 Not started` | `1 sprint`

Any C++ type can be exposed to Lua through a registration macro or annotation, with automatic binding of constructors, methods, properties, and enums. Scripts can call C++ functions and C++ can call script functions. The binding layer handles type marshalling, lifetime management (preventing dangling references to destroyed entities), and error propagation.

### 10.6 Threading API (from scripts)
`🔲 Not started` | `1 sprint`

Scripts can create background threads that run Lua code in parallel with the main game loop. Thread-safe communication uses a message queue rather than shared memory. A dedicated `Thread` type in Lua exposes start, join, wait, and is_running operations. Used for procedural generation, pathfinding batches, and heavy computation that should not block the frame.

### 10.7 Coroutines / async (wait, yield, sequences)
`🔲 Not started` | `1 sprint`

Lua coroutines integrated as first-class engine tools. A script can `yield` for a number of seconds (`wait(2.5)`), yield until a condition is true (`wait_until(health < 0)`), or chain actions into a sequence (`sequence(wait(1), play_sound, wait(2), spawn_enemy)`). Coroutines are automatically resumed each frame and can be cancelled or paused externally.

### 10.8 Group system (get entities by tag/group)
`🔲 Not started` | `1 sprint`

Entities can be added to named groups at runtime or in the editor. The global group API provides `find_group("enemies")` returning all entities in that group, and `call_group("enemies", "take_damage", 10)` calling a method on each member. Groups are more dynamic than tags and are the primary way scripts discover related entities without hard references.

### 10.9 Native extension system (C/C++/Java/JS plugins)
`🔲 Not started` | `2-3 sprints`

A plugin framework for extending the engine with native code without modifying the engine source. Extensions are compiled as shared libraries (`.so` / `.dll` / `.dylib`) and loaded at runtime. The SDK provides a C API for registering components, systems, asset types, and editor panels. Used for platform-specific integrations (ads, IAP, push notifications), high-performance libraries, and custom hardware access.

### 10.10 Visual scripting (blueprint for designers)
`🔲 Not started` | `3-4 sprints`

A node-based visual scripting language for game designers who prefer not to write text code. Nodes represent actions (spawn, move, play sound, show UI), conditions (if health < 0, if key pressed), and variables. Wires connect outputs to inputs. The graph is saved as a JSON asset and interpreted or compiled to Lua. Supports custom node creation from existing Lua scripts.

---

## Phase 11 — Debugger + Profiler

### 11.1 Script debugger (breakpoints, step, variable watch)
`🔲 Not started` | `2-3 sprints`

A full-featured debugger for Lua scripts integrated into the editor. Supports breakpoints (line and conditional), step over, step into, step out, continue, and pause. A variable watch panel displays local, upvalue, and global variables with expandable tables. The call stack is shown with file/line for each frame. Debugging can be attached to a running game (editor or standalone build) via a TCP connection.

### 11.2 CPU profiler (Tracy)
`🔲 Not started` | `1-2 sprints`

Integration with Tracy Profiler, a high-performance frame profiler with nanosecond-resolution instrumentation. Zones are automatically added by the engine's core systems (render, physics, update, script) and can be added by scripts. The profiler displays a timeline view, frame time histogram, call-stack tree with inclusive and exclusive costs, and per-zone statistics. Remote capture connects to a running game over the network.

### 11.3 GPU profiler (draw calls, shader info, timestamps)
`🔲 Not started` | `1-2 sprints`

GPU performance data collected through OpenGL timestamp queries and vendor extensions (AMD/GPUPerfStudio, NVIDIA/Nsight). Captures per-frame draw call count, shader switch count, vertex/index buffer bind count, texture bind count, and GPU stall reasons. A timeline view shows GPU work overlapping with CPU work to identify synchronization bottlenecks.

### 11.4 Memory profiler (snapshots, leaks, fragmentation)
`🔲 Not started` | `1-2 sprints`

Tracks all engine allocations through a custom allocator interface, categorizing memory by system (render, physics, audio, script, assets). Can take snapshots of the entire heap at any point and compare two snapshots to identify leaks and growing allocations. Displays per-asset memory usage (texture resolution, mesh vertex count, audio duration) in a sortable table.

### 11.5 Frame debugger (capture frame, inspect draw calls)
`🔲 Not started` | `2-3 sprints`

Captures a single frame of rendering commands and lets you step through each draw call. For each call, shows the shader, material, mesh, textures, render state (blend, depth, cull), MVP matrix, and bounding box. Overlays the affected geometry in the viewport. Supports filtering by object or shader and displays the pixel history at a selected screen location.

### 11.6 Console command + autocomplete
`🔲 Not started` | `1 sprint`

A developer console accessible in-game (toggle with backtick/tilde) and in the editor. Commands include cvar get/set, script execution, scene loading, entity manipulation, and performance toggles. Autocomplete suggests commands and cvars with fuzzy matching. A command history is preserved across sessions. Commands can be registered from C++ or from scripts through a simple API.

### 11.7 Custom performance monitors
`🔲 Not started` | `1 sprint`

Users can register their own named performance counters from scripts and C++, which appear alongside the built-in profiler stats. A counter can be a scalar (frame time for AI update), a histogram (memory pool usage), or a sampled value (player position over time). Counters are visualized in the profiler overlay and logged to file for post-mortem analysis.

### 11.8 Remote inspector + live camera replication
`🔲 Not started` | `2 sprints`

A networked debugging tool that connects the editor to a running game on another device (phone, console, another PC). The editor displays the remote scene hierarchy, entity component values, and the game's log output in real-time. The game's camera can be replicated in the editor viewport, letting you see exactly what the player sees. Changes made in the editor's inspector apply to the remote game.

### 11.9 Profiling overlay (in-game stats)
`🔲 Not started` | `1 sprint`

A configurable on-screen display rendered by ImGui showing live performance stats while the game runs. Includes FPS, frame time (CPU + GPU), draw calls, triangles, entities, physics bodies, audio voices, memory usage, and custom counters. Users can toggle individual stats on/off and choose graph vs text display. The overlay is themed to match the game's UI and can be disabled in release builds.

### 11.10 Recorder (video capture tool)
`🔲 Not started` | `1-2 sprints`

Captures the game viewport to a video file with synchronized audio. Uses GPU readback (glReadPixels or pixel-buffer-object async read) for minimal performance impact. Supports multiple codecs (PNG sequence, lossless, H.264/NVENC if available), configurable resolution and framerate, and optional time-lapse mode.

### 11.11 Profile Analyzer (deep analysis, comparison)
`🔲 Not started` | `1 sprint`

A post-hoc profiling analysis tool that loads saved profile data from multiple capture sessions. Compares frames from different builds or settings ("before" vs "after optimization") and highlights regressions and improvements. Can aggregate data over thousands of frames to identify statistical outliers and frame-time spikes. Generates a report with charts and recommendations.

### 11.12 Code Coverage (test coverage reporting)
`🔲 Not started` | `1 sprint`

A tool that instruments the test suite to track which engine code paths are exercised by the tests. Generates HTML reports showing per-file and per-function coverage percentages with source-code highlighting of covered/uncovered lines. Integrates with the CI pipeline to enforce minimum coverage thresholds on new code.

---

## Phase 12 — 2D Tools

### 12.1 Sprite / flipbook animation
`🔲 Not started` | `1-2 sprints`

A 2D sprite component that renders a texture (or sub-region of an atlas) as a flat quad in the 3D world or in an orthographic 2D layer. Flipbook animation plays a sequence of atlas frames at a configurable framerate with optional looping, ping-pong, and frame callbacks. Sprites support colour modulation, alpha cutoff, material overrides, and flip X/Y.

### 12.2 Atlas packing (multi-resolution)
`🔲 Not started` | `1-2 sprints`

An atlas packing tool that takes a folder of individual sprite images and packs them into a single texture atlas with a metadata file describing each sprite's UV rect. Supports automatic padding, bleeding (pixel duplication at edges to prevent seams), and multi-resolution atlas generation for different screen densities (1x, 2x, 3x).

### 12.3 TileMap editor (2D tile levels)
`🔲 Not started` | `2-3 sprints`

A 2D tile-based level editing system. A tilemap consists of multiple layers (ground, objects, collisions, decoration) with per-cell tiles selected from a tileset. The editor provides paint, fill, line, rectangle, and random brushes. Auto-tiling rules connect tile variants based on adjacency. A collision layer automatically generates physics collision shapes.

### 12.4 Parallax layers (pseudo-3D scrolling)
`🔲 Not started` | `1 sprint`

Multiple background layers that scroll at different speeds relative to the camera, creating a sense of depth. Each layer has a scale factor and an optional motion mirror (horizontal, vertical, both). Supports layer ordering (front to back), individual layer visibility toggles, and infinite scrolling for looping backgrounds.

### 12.5 2D lighting (normal maps, shadows)
`🔲 Not started` | `1-2 sprints`

A 2D lighting system with point, directional, and spot lights that interact with sprites using normal maps and specular maps. Shadows are cast by sprites with an occluder shape and rendered with a real-time SDF (signed-distance field). Supports soft shadows, coloured lights, ambient light, and emissive sprites.

### 12.6 2D physics (Box2D-like)
`🔲 Not started` | `2 sprints`

A dedicated 2D physics engine (Box2D or a modern equivalent) for 2D-only games where full 3D physics would be overkill. Supports rigid body dynamics, collision shapes (rectangle, circle, polygon, edge chain), joints (revolute, prismatic, distance, weld, wheel, motor), and triggers. Runs in 2D space independently of the 3D physics system.

### 12.7 Pixel Perfect rendering
`🔲 Not started` | `1 sprint`

A rendering mode that snaps all sprite rendering to pixel-grid coordinates, eliminating sub-pixel blurring in pixel-art games. Uses integer scaling with configurable scale factor (1x, 2x, 3x), a viewport resolution locked to the game's native resolution, and nearest-neighbour filtering. A debug overlay shows the "virtual pixel grid."

### 12.8 Aseprite/PSD importer
`🔲 Not started` | `1 sprint`

Direct import of Aseprite (`.aseprite` / `.ase`) files and Adobe Photoshop (`.psd`) files into the engine. Layers, frames, tags, and slice data are preserved and converted to sprites, animations, and flipbooks. The importer is live: saving the source file in the external editor triggers an automatic reimport into the engine.

---

## Phase 13 — Game Services

### 13.1 Localization (PO/CSV, tr(), plural, RTL, pseudolocalization)
`🔲 Not started` | `2-3 sprints`

A complete localization system supporting gettext `.po` files and CSV spreadsheets. The `tr()` function (or equivalent) looks up the current locale's translation for a given string key. Supports plural forms (with locale-specific plural rules — 2 forms for English, 3+ for Slavic languages), gender, and translation contexts. RTL text (Arabic, Hebrew) is auto-detected and mirrored. A pseudolocalization mode ("Pseudo — •[this is what it will look like]•") tests UI layout without real translations.

### 13.2 Save/load game state (local + cloud)
`🔲 Not started` | `2 sprints`

A binary serialization system that captures the complete game state — entity positions, health, inventory, quest flags, settings — into a save file. Supports multiple save slots, auto-save (triggered at checkpoints, on pause, periodically), and metadata (timestamp, play time, screenshot thumbnail). Cloud save uploads to a backend service (Steam Cloud, custom server) and syncs across devices.

### 13.3 Game flow (menu → load → play → pause → gameover)
`🔲 Not started` | `2 sprints`

A scene-driven game flow machine that manages the transition between game states: boot logo, main menu, options, loading screen, gameplay, pause menu, game over, credits, and quit. Each state loads the appropriate scene(s), configures input context (UI vs gameplay), and manages time scale (pause freezes gameplay timers but not UI). Extensible with custom states.

### 13.4 Sequencer / cutscenes (Timeline-like)
`🔲 Not started` | `2-3 sprints`

A timeline-based cutscene authoring tool where animations, camera movements, dialogue, sound effects, and script events are placed on tracks with precise timing. The sequencer runs in a dedicated "cinematic" game state that takes control of the camera and player. Supports subtitles, black bars (letterbox), and skip-to-end with state restoration.

### 13.5 IAP (Google, Apple, Amazon, Steam)
`🔲 Not started` | `1-2 sprints`

In-app purchase integration for all major app stores through a unified API. The game asks for a product list, initiates a purchase flow, and receives a receipt for validation. Consumable, non-consumable, and subscription products are supported. Receipts can be validated client-side or server-side for anti-tampering.

### 13.6 Ads (AdMob, IronSource, Unity Ads)
`🔲 Not started` | `1-2 sprints`

Ad network integration through a common API abstraction. Supports banner ads (fixed position, smart banner), interstitial ads (full-screen between levels), rewarded video ads (player opts in for a reward), and native ads (custom-rendered in the UI). Each ad placement is configured per-platform and can be remotely toggled.

### 13.7 Analytics (Firebase, custom)
`🔲 Not started` | `1 sprint`

An analytics system that sends events (level start, level complete, purchase, custom) to a backend (Firebase, custom server, or both). Events are batched and sent in the background to avoid frame impact. A debug view shows live event traffic and validates event schemas. Supports user segmentation, funnels, and retention tracking.

### 13.8 Push notifications
`🔲 Not started` | `1 sprint`

Local and remote push notifications for mobile platforms. Local notifications are scheduled from the game (remind player to return). Remote notifications are sent from a server (new content, promotions). The notification payload can include custom data that the game reads on cold start to navigate directly to the relevant screen.

### 13.9 Remote Config (A/B testing)
`🔲 Not started` | `1-2 sprints`

A remote key-value store that overrides local configuration at runtime without a game update. Used for A/B testing (different difficulty curves, pricing, UI layouts), feature flags (enable a new system for 10% of players), and emergency toggles (disable a crashing feature remotely). Config values are cached locally and refreshed on launch.

### 13.10 Cloud Save (cross-save)
`🔲 Not started` | `1-2 sprints`

Saves the player's game state to a server, enabling cross-device and cross-platform save synchronization. Handles conflict resolution (last-write-wins or manual conflict UI with timestamp and slot preview). Integrates with the existing save/load system so the game only needs to call save, and the sync happens transparently in the background.

### 13.11 Cloud Code (server-side logic)
`🔲 Not started` | `1-2 sprints`

A serverless hosting environment for running game logic on the backend — used for authoritative validation, matchmaking state, economy transactions, and scheduled events. Code is written in Lua (like the client), uploaded via a CLI or CI pipeline, and versioned. The client calls cloud functions via the messaging system with serialized arguments.

### 13.12 Economy (virtual currency, shop)
`🔲 Not started` | `1-2 sprints`

A virtual economy system with soft currency (earned through gameplay), hard currency (purchased with real money), and inventory items. The economy is validated server-side to prevent cheating. Supports bundles, discounts, daily rewards, and limited-time offers. The shop UI is generated from dynamic product data rather than hardcoded layouts.

### 13.13 Authentication + Player Accounts
`🔲 Not started` | `1-2 sprints`

Player identity and account management. Supports anonymous login (auto-generated player ID), social login (Google, Apple, Steam, Facebook), and email/password authentication. The account stores persistent data (username, avatar, settings, purchases). An authentication token is used for all subsequent API calls and networking sessions.

### 13.14 Friends + Leaderboards
`🔲 Not started` | `1-2 sprints`

A social graph system shared across all services. Players can add friends, view friend lists, see online status, and join friends' games (invite system). Leaderboards support friend-only and global scopes, timestamped entries, and metadata per entry (character used, loadout). Leaderboards are queried with pagination and can filter by time window (daily, weekly, all-time).

### 13.15 User Reporting + Moderation
`🔲 Not started` | `1 sprint`

In-game tools for players to report inappropriate behaviour, content, or bugs. Reports include contextual data (player ID, timestamp, session info, screenshot) and are submitted to a moderation dashboard. The moderation API supports automated rule enforcement (ban/kick by criteria) and manual review workflows.

### 13.16 Cloud Diagnostics (crash reporting)
`🔲 Not started` | `1-2 sprints`

Collects crash dumps, errors, and performance metrics from running games and uploads them to a dashboard for analysis. Crashes are symbolicated so the stack trace shows function names and line numbers. Errors (assertions, failed Result returns) are captured as breadcrumbs leading up to the crash. A "session replay" optionally captures the last N seconds of input.

### 13.17 Game backends (Nakama, PlayFab, Photon, Steamworks)
`🔲 Not started` | `2-3 sprints`

Abstracted integration interfaces for popular game backend providers, so the game code does not need to change when switching providers. Nakama provides open-source matchmaking, chat, and storage. PlayFab manages live ops and economy. Photon provides real-time multiplayer relay. Steamworks wraps Steam achievements, cloud, multiplayer, and workshop APIs.

### 13.18 Version Control integration
`🔲 Not started` | `1 sprint`

A panel within the editor that shows the version control status of the current project: modified files, staged/unstaged changes, commit history, and branch. Supports Git and Plastic SCM. Common operations (commit, push, pull, branch switch, diff) are available without leaving the editor. Lock indicators show which assets are exclusively checked out by other team members.

---

## Phase 14 — Export & Pipeline

### 14.1 Asset baking (textures, meshes, audio optimize)
`🔲 Not started` | `2-3 sprints`

An offline build step that converts source assets into engine-optimized runtime formats. Textures are compressed, mipmaps generated, and packed into GPU-ready blobs. Meshes are optimized for cache-friendly vertex/index buffer layouts, tangents computed, and LOD levels generated. Audio is resampled to the project's target sample rate, compressed, and normalized. Baked assets are deterministic (same input → same output) and cached.

### 14.2 Resource packing (.pak, encrypted or not)
`🔲 Not started` | `1-2 sprints`

All baked assets are bundled into one or more `.pak` archive files for distribution. The archive format supports fast seeking (index at the head), optional compression (zstd, LZ4), optional encryption (AES-256), and integrity verification (SHA-256). PAK files can be mounted, unmounted, and prioritized at runtime, enabling mods and DLC.

### 14.3 Export templates (release build without editor)
`🔲 Not started` | `2 sprints`

The engine can be built as a standalone executable that runs a game project without the editor. The export process creates a minimal runtime: the engine's binary is stripped of editor-only code, linked against the game's scripts and packaged assets, and configured for release mode (optimizations on, symbols off, asserts disabled). Platform-specific export templates (Windows, Linux, Android, etc.) are built separately.

### 14.4 App manifest (compile out unused features)
`🔲 Not started` | `1-2 sprints`

A configuration file that declares which engine modules the game actually uses. At build time, any system not listed (e.g., physics for a visual novel, audio for a tools project) is compiled out of the runtime binary, reducing binary size and attack surface. The manifest can be auto-generated by the editor based on a project scan.

### 14.5 Launcher (graphics config, API selection)
`🔲 Not started` | `1-2 sprints`

A small application that runs before the game, presenting graphics options (resolution, display mode, vsync, quality preset, render API selection) and allowing the user to launch the game with those settings. Supports command-line arguments for automation and CI. The launcher also performs system checks (GPU capabilities, driver version, available memory) and warns about missing requirements.

### 14.6 Live update / content streaming
`🔲 Not started` | `2 sprints`

The initial game download contains only essential content. Additional content (HD textures, later levels, language packs) is downloaded on demand or in the background while the player plays. The live update system uses the same `pak` mounting infrastructure, so new archives are downloaded and hot-mounted without restarting. Progress is shown with a persistent UI notification.

### 14.7 Bundle splitting (DLC, mods, priority archives)
`🔲 Not started` | `1-2 sprints`

Game content is logically split into named bundles: base game, free update, expansion DLC 1, HD texture pack, etc. Each bundle is a separate `pak` file with its own version and signing. The engine mounts bundles in priority order (base → official DLC → mods → local override) so that higher-priority files replace lower-priority ones seamlessly.

### 14.8 Scriptable Build Pipeline (custom, incremental, cached)
`🔲 Not started` | `2-3 sprints`

A programmable build pipeline where each build step is a script (Lua or C++ plugin) that transforms inputs to outputs. Steps are automatically parallelized across available CPU cores. The pipeline is incremental — only changed inputs are reprocessed — with a content-addressable cache that avoids re-baking across branches and machines.

### 14.9 Patcher / updater (delta patching)
`🔲 Not started` | `1-2 sprints`

When a new version of the game is released, the patcher downloads only the changed bytes (binary diff) rather than the entire game. Patches are generated on the build server using bsdiff or a similar delta algorithm. The patcher applies the delta locally, verifies integrity, and replaces the old files. Supports rollback to the previous version.

### 14.10 Movie maker mode (render video with audio)
`🔲 Not started` | `1-2 sprints`

A special engine mode that renders the game (or a specific scene) to a video file at a fixed timestep with deterministic frame pacing. Each frame is captured from the GPU and encoded with audio synchronization. Useful for creating trailers, cinematics, and marketing material directly from the engine. Supports multi-sample accumulation for anti-aliased output at higher resolutions.

### 14.11 WebGL Publisher (one-click web deploy)
`🔲 Not started` | `1 sprint`

A build target that exports the game as a WebGL application (HTML + JS + WASM) and optionally publishes it to a web host via FTP/SSH/itch.io API in one click. Includes an HTML template with customizable loading screen, progress bar, and embed options. The publisher can also generate an embeddable iframe code for sharing on game portals.

### 14.12 Sysroot + toolchains (cross-compilation)
`🔲 Not started` | `1 sprint`

Provides the sysroots and cross-compilation toolchains needed to build the engine for platforms other than the host. A single CMake invocation can produce builds for Windows, Linux, Android, and Web simultaneously. Toolchains are versioned and reproducible, ensuring that builds from any machine produce identical output.

---

## Phase 15 — Multiplayer

### 15.1 Low-level transport (ENet / WebSocket / WebRTC)
`🔲 Not started` | `2 sprints`

A socket abstraction layer providing reliable/unreliable, ordered/unordered, and sequenced messaging over UDP (ENet), TCP (WebSocket), and peer-to-peer (WebRTC) transports. Handles connection establishment, keep-alive, fragmentation, reassembly, and congestion control. The transport layer is selected per-platform: ENet for desktop, WebSocket for web builds, WebRTC for browser-to-browser.

### 15.2 Netcode / RPC replication (client-server)
`🔲 Not started` | `3-4 sprints`

A high-level netcode system where the server is authoritative and clients predict and reconcile. Remote Procedure Calls (RPCs) mark specific functions for network execution (on server, on clients, on all). State synchronization is automatic for networked components (position, rotation, health) with delta compression, interest management (relevancy based on distance), and bandwidth budgeting.

### 15.3 Dedicated server (headless + multiplayer logic)
`🔲 Not started` | `2 sprints`

A headless build of the engine that runs as a dedicated game server with no graphics, audio, or input subsystems. The server runs the full simulation, receives client inputs, and broadcasts state updates. Can be hosted on bare metal, in containers, or via dedicated game server hosting services. Supports console input for admin commands.

### 15.4 Lobby + Matchmaker
`🔲 Not started` | `2-3 sprints`

A lobby system where players create or join game sessions with configurable properties (max players, map, game mode, skill level). The matchmaker uses a rules engine to pair players based on skill rating (ELO or TrueSkill), latency, party size, and region. Supports quick play (auto-match), invite-only lobbies, and spectator slots.

### 15.5 Relay (NAT punchthrough)
`🔲 Not started` | `1-2 sprints`

A relay service that facilitates peer-to-peer connections when direct NAT traversal fails. The relay server receives UDP packets from each peer and forwards them, acting as a fallback when STUN/TURN hole-punching is blocked by symmetric NATs or firewalls. Relay bandwidth is optimized by only forwarding to relevant peers based on interest management.

### 15.6 Game server hosting (Multiplay-like)
`🔲 Not started` | `2-3 sprints`

Integration with dedicated game server hosting providers that spin up server instances on demand when a match is found. The server lifecycle is managed: allocate on match start → warm-up → game loop → teardown on match end. Servers report health, player count, and performance metrics back to the matchmaker for fleet management.

### 15.7 Voice chat (Vivox-like)
`🔲 Not started` | `2 sprints`

Real-time voice communication integrated with the game's lobby and party systems. Players are placed into voice channels (team chat, proximity chat, global). Features include push-to-talk, voice activity detection, per-player volume control, mute, and deafen. Audio encoding uses Opus for low-latency high-quality transmission. Echo cancellation and noise suppression are built in.

### 15.8 Anti-cheat
`🔲 Not started` | `2-3 sprints`

A multi-layered anti-cheat system. Server-side: validates player state (position, health, inventory) against the authoritative simulation and flags anomalies. Client-side: integrity checks on the engine binary, script files, and asset hashes. Memory scanning detects common cheat tools. Reports suspected cheaters with evidence (screenshots, state delta logs) for manual review.

### 15.9 Bandwidth profiler + network stats
`🔲 Not started` | `1 sprint`

An in-game overlay showing network performance: round-trip time, packet loss, bandwidth up/down, objects being replicated, and RPC frequency. Helps developers identify bandwidth hogs, optimize replication frequency, and debug lag. Records a network trace to a file for offline analysis.

### 15.10 Replay system (spectate, demos)
`🔲 Not started` | `2 sprints`

Records all inputs and random seeds during a game session into a compact demo file. The demo can be replayed from any camera angle (free spectator, follow-player, fixed), rewound, and fast-forwarded. Used for fair-play verification in competitive games, memorial clips, and debugging. The replay is deterministic: playing the same input file always produces the same output.

---

## Phase 16 — Web + Cross-platform

### 16.1 Web / WASM (Emscripten + WebGL/WebGPU)
`🔲 Not started` | `3-4 sprints`

The engine is compiled to WebAssembly via Emscripten, running in a browser with WebGL 2.0 or WebGPU. The platform abstraction layer adds a Web backend that maps input, audio, and windowing to browser APIs (Gamepad API, WebAudio, Canvas/HTML). Asset loading uses IndexedDB for caching and streaming HTTP for initial download. A custom HTML shell provides JavaScript interop for browser-specific features (URL parameters, fullscreen API).

### 16.2 WebGPU backend (Vulkan-like, modern)
`🔲 Not started` | `4-6 sprints`

A new render device backend targeting WebGPU (via Dawn or wgpu), providing a modern, low-overhead graphics API that runs natively on Vulkan, Metal, and DirectX 12, plus on the web via WebGPU. This becomes the primary render path long-term, replacing direct OpenGL calls with a more portable, explicit, and multithreading-friendly abstraction. Falls back to OpenGL on older hardware.

### 16.3 Mobile (Android, iOS — touch input, gestures)
`🔲 Not started` | `3-4 sprints`

The engine builds and runs on Android (via NDK) and iOS (via Xcode toolchain). The platform layer handles mobile lifecycle (pause/resume, low memory warnings, orientation changes). Touch input supports multi-touch, gestures (tap, swipe, pinch, long-press), and force touch. The render device uses OpenGL ES 3.2 or Metal (via WebGPU abstraction). Performance tuning profiles for mobile GPU architectures (tiled renderers, unified memory).

### 16.4 XR (OpenXR — Quest, Valve Index, Pico, Apple Vision Pro)
`🔲 Not started` | `3-4 sprints`

Full OpenXR integration for VR and AR head-mounted displays. The render device creates a dedicated eye render target for each display, applies the HMD's projection and view matrices, and submits to the OpenXR compositor. Supports room-scale tracking, hand-held controllers, haptic feedback, and pass-through AR (on Quest and Apple Vision Pro).

### 16.5 XR Interaction Toolkit (grab, teleport, UI in VR)
`🔲 Not started` | `2-3 sprints`

A high-level interaction framework built on top of OpenXR. Provides grab (pick up objects with physics-based hand interaction), teleport (point-and-click locomotion with arc visualization), snap-turn, UI interaction (laser pointer or touch for virtual screens), socket (place objects at designated spots), and climbing. Interactions are configured in the inspector with no code.

### 16.6 XR Hands (hand tracking)
`🔲 Not started` | `2 sprints`

Hand tracking without controllers, using the headset's cameras to track hand and finger positions. The hand skeleton drives an animated hand mesh with natural finger curling, pinching, and pointing. Hand interactions replace controller interactions: grab with pinch, push buttons with poke, gesture recognition for menu activation.

### 16.7 Console platforms (PS5, Xbox, Switch — via licensing)
`🔲 Not started` | `4-6 sprints`

Platform-specific backends for console SDKs. This requires licensing agreements with each platform holder. The engine architecture anticipates console ports through the existing platform/render/input abstraction layers. Each console adds a new backend set: platform (achievements, save files, friends list), render (Gnm on PS5, DirectX 12 on Xbox, NVN on Switch), and input (DualSense, Xbox Wireless, Joy-Con).

### 16.8 Linux/macOS native packaging
`🔲 Not started` | `1-2 sprints`

Native packaging for Linux (AppImage, Flatpak, Snap, .deb) and macOS (.dmg, signed and notarized). The build pipeline produces platform-specific binaries with the correct icon, metadata, and code signing. On macOS, the bundle includes the correct `Info.plist`, hardened runtime entitlements, and notarization ticket for Gatekeeper compatibility.

---

## Phase 17 — AA Polish & Scale
*The difference between "it works" and "it's professional."*

### 17.1 Job system multithread (task scheduler, fibers, lock-free)
`🔲 Not started` | `2-3 sprints`

A high-performance job scheduler that distributes work across all available CPU cores. Jobs are small, dependency-tracked units of work with a graph-based scheduler (fork-join). Uses a fiber-based model for coroutine-like job suspension without OS thread overhead. Lock-free queueing and work-stealing keep all cores busy. The job system replaces manual threading in physics, animation, particle updates, and asset loading.

### 17.2 ECS performant (DOTS-like, archetypes, chunk storage)
`🔲 Not started` | `3-4 sprints`

A high-performance Entity Component System based on archetypes (entities with the same component types are stored contiguously in chunks). Queries iterate only matching archetypes with cache-friendly memory access patterns. The system is optional — scenes can continue to use the existing lightweight ECS — but for performance-critical systems (many enemies, bullets, physics debug), the high-perf ECS provides order-of-magnitude speedups.

### 17.3 Burst compiler (SIMD JIT for hot paths)
`🔲 Not started` | `2-3 sprints`

A just-in-time compiler that translates a subset of the engine's hot-path operations into highly optimized SIMD machine code. Focused on math-heavy loops: skinning vertices, particle simulation, physics constraint solving, and transform hierarchy updates. The burst pipeline takes annotated C++ functions and compiles them at startup to native code with auto-vectorization.

### 17.4 Occlusion culling (Umbra / software rasterization)
`🔲 Not started` | `2 sprints`

An occlusion culling system that prevents rendering objects hidden behind other geometry. Uses hardware occlusion queries (conditional rendering) and/or software rasterization of a hierarchical Z-buffer. A dedicated compute pass generates a low-resolution depth buffer from occluders, then tests each object's bounding box against it. Objects behind fully opaque occluders are skipped at the CPU level.

### 17.5 LOD system (automatic, impostors, cross-fade)
`🔲 Not started` | `2 sprints`

Level-of-detail system that reduces geometric complexity with distance from the camera. LOD levels are imported from glTF, generated automatically (mesh decimation at bake time), or replaced with impostor billboards for distant objects. LOD transitions use dither cross-fade (alpha dithering for no-cost blending) or geometric morphing. Per-object screen-size thresholds are configurable.

### 17.6 Volumetric fog + clouds
`🔲 Not started` | `2 sprints`

A full volumetric fog system using a 3D froxel grid that is raymarched per pixel. Reacts to all light types with shadowing and supports custom density from a 3D texture. Layered clouds use a tileable noise-based density field with coverage, altitude, and sharpness parameters. Clouds cast shadows on the ground and receive light from the sun and sky.

### 17.7 Water system (planar reflection, refraction, waves)
`🔲 Not started` | `2-3 sprints`

A water rendering system supporting lakes, oceans, and rivers. Gerstner wave simulation drives vertex displacement for animated wave crests. Planar reflection renders the scene mirrored below the water plane. Refraction samples the underwater scene with a distorted UV offset. Foam appears at wave crests and shorelines. Depth-based colour blending transitions from clear to deep blue.

### 17.8 Terrain system (heightmap, splatmap, vegetation, LOD)
`🔲 Not started` | `3-4 sprints`

A terrain rendering system for large outdoor environments. A heightmap defines the base geometry. A splatmap blends up to 8 material layers (grass, rock, snow, sand, mud, etc.) with per-pixel control. Vegetation is instanced on the terrain surface — grass, bushes, trees — with GPU instancing and LOD. The terrain uses clipmap LOD (higher tessellation near camera) and supports runtime height editing via the editor tools.

### 17.9 Destruction system (fracture, debris, physics shards)
`🔲 Not started` | `2-3 sprints`

A pre-fractured destruction system: a mesh is pre-processed into fragments at asset-import time, and at runtime, a trigger (explosion, impact) replaces the intact mesh with its fragments, each turned into a physics rigid body. Debris particles spawn from fragment creation. The fracture pattern can be random, Voronoi-based, or hand-authored. Supports progressive damage (cracks before full break).

### 17.10 Video playback (Bink / Ogg Theora)
`🔲 Not started` | `1-2 sprints`

Full-screen and texture-sampled video playback. Uses Bink (the industry standard for game video, via RAD Game Tools) or Ogg Theora (open source) as codec backends. Video is decoded on a worker thread and presented as a GPU texture. Supports playback control (play, pause, seek, loop), subtitle overlay, audio track selection, and alpha-channel video (for UI backgrounds and transitions).

### 17.11 Modding support (PCK + native extension API + Steam Workshop)
`🔲 Not started` | `2-3 sprints`

The engine supports user-created modifications. Mods are packaged as standalone `.pak` archives that override or extend base game assets and scripts. The modding API is an extended subset of the engine's C++ and Lua APIs, versioned and documented for mod authors. Steam Workshop integration provides listing, subscribing, downloading, and updating mods.

### 17.12 ML-Agents (reinforcement learning for AI)
`🔲 Not started` | `2-3 sprints`

A framework for training game AI through reinforcement learning, Unity ML-Agents style. The game exposes observations (player position, health, enemy states) and actions (move, shoot, jump) as a Gym-like environment. A Python training harness connects to the running game, runs episodes with rewards, and exports trained neural network weights. The trained model runs in-game as an NPC brain.

### 17.13 Live Capture (motion capture from iPhone/device)
`🔲 Not started` | `2 sprints`

Streams motion capture data from an iPhone or iPad (using ARKit face tracking and body tracking) or a Perception Neuron suit directly into the animation system in real-time. The mocap data is applied to the character's skeleton with optional smoothing and retargeting. Useful for rapid animation prototyping, live performance-driven characters, and facial capture for dialogue.

### 17.14 Adaptive Performance (thermal/quality scaling)
`🔲 Not started` | `1-2 sprints`

Monitors device temperature, CPU/GPU load, and frame time, and dynamically scales quality settings (resolution scale, shadow distance, particle count, LOD bias) to maintain a stable framerate and prevent thermal throttling. On mobile, integrates with vendor APIs (Android Performance Tuner, Apple Metal Performance HUD). Visual quality slowly degrades before the device overheats, avoiding sudden frame drops.

### 17.15 Dedicated tools (Bunnymark benchmark, stress tests)
`🔲 Not started` | `1 sprint`

A set of benchmarking and stress-test tools integrated into the engine. The Bunnymark graphically demonstrates how many animated sprites the engine can render at 60 FPS. Stress tests push specific subsystems (10k physics bodies, 500k particles, 1000 lights, 50 bones each on 100 characters) to find regression and measure performance scaling across hardware configurations.

---

## Priority Recommendation — Next 6 Features

Based on dependency analysis and maximum force-multiplier effect:

| Rank | Feature | Phase | Rationale |
|---|---|---|---|
| 1 | **Logging system** | 0 | Nothing is structured; every future feature needs logging |
| 2 | **ImGui + debug overlay** | 0 | Force multiplier — all future debugging goes through this |
| 3 | **Input action system** | 1 | Current SDL raw binding is not rebindable; blocks editor UX |
| 4 | **Config system + cvars** | 0 | All graphics/audio settings depend on this |
| 5 | **Scene serialization** | 1 | Without this, nothing can be persisted |
| 6 | **Physics (Jolt)** | 3 | Unlocks collisions, raycasts, gameplay — core game feel |

---

## Legend

| Symbol | Meaning |
|---|---|
| ✅ Done | Feature is implemented and tested |
| 🔄 In progress | Currently being worked on via SDD workflow |
| 🔲 Not started | Not yet specified or implemented |
| ⏸️ Blocked | Blocked by another feature or decision |
| ☑️ Deferred | Postponed to a later phase |

Sprint = 1 week of SDD workflow (spec → critique → contract → critique → human approval → code → review → ADR/wiki/governance).
