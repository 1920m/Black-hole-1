# NEA Writeup — Kerr Black Hole Simulation (RSE)
## OCR Computer Science A-Level Non-Examined Assessment (2026)
**Candidate:** Moga, Cris  
**Centre Number:** 58319  
**Project:** Relativistic Sandbox Engine — real-time physically-accurate Kerr black hole simulation

---

## Document Structure

This project has **six iterations** in total:

| File | Iterations | Content |
|---|---|---|
| `writeup/iteration_1_existing_project.md` | **Iteration 1** | The existing RSE project — demonstrated, tested, found lacking. The review motivates the full rebuild. **Read this first.** |
| `writeup/writeup.md` (this file) | **Iterations 2–6** | The rebuild — Physics library, Kerr CPU tracer, Disk physics, Vulkan GPU, Sgr A* calibration, Visual presets. |

The NEA narrative runs as a single story: Iteration 1 shows what was attempted; Iterations 2–6 show how the rebuild addressed every failure found in the review.

---

## How to Use This Document

This document records the design, development, testing and evaluation of the project at every stage of every iteration. It captures the narrative, diagrams, pseudocode, test tables, screenshots, and stakeholder reviews that make up the submitted write-up.

**The golden rule from OCR:** write up as you go. Do not code everything and then write. The write-up must narrate the journey in real time, including failures, fixes, and design changes.

### Screenshot Policy (critical for marks)

Screenshots are taken at the following moments and embedded in the write-up. Each is saved to `writeup/screenshots/iteration_N/` with a descriptive filename (e.g. `iter1_cmake_config.png`, `iter1_ctest_gate0_pass.png`):

- **Any new class or module implemented** — screenshot of the header/class definition and a brief run to confirm compilation.
- **Any error that occurs** — screenshot of the terminal error output **before fixing it** (the error is evidence — do not fix it and forget to document it).
- **The fix** — screenshot of the changed code and the terminal showing the error is resolved.
- **Any test result** — screenshot of `ctest` or Catch2 output (pass or fail).
- **Any rendered output** — screenshot of the visual result (image file or live window).
- **Git tag applied** — after every gate tag, run `git log --oneline --decorate -5` and screenshot the terminal. This proves version control was used throughout development.
- **End-of-iteration prototype** — full-screen screenshot or Screencastify video showing the working state at the end of the iteration.

All videos must be recorded with Screencastify, saved to Google Drive, and linked using `drive.google.com` links — **never** `watch.screencastify.com` links.

All images in the write-up must be labelled with a figure number and a caption.

### Git Versioning Policy (mandatory)

This project uses **git tags** to preserve a runnable snapshot at the end of every iteration. Any past iteration can be restored with `git checkout <tag>` — no separate folders needed.

**Tag names to apply at each gate:**

| Gate | Tag | Trigger |
|---|---|---|
| Review | `iteration-1-complete` | Existing RSE project reviewed, rebuild decision documented |
| Gate 0 | `iteration-2-gate-0` | All three Catch2 test suites green |
| Gate 1 | `iteration-2-complete` | CPU Schwarzschild shadow PNG saved |
| Gate 2 | `iteration-3-gate-2` | Kerr θ-oscillation confirmed |
| Gate 3 | `iteration-3-complete` | Lopsided Kerr shadow PNG saved |
| Gate 4 | `iteration-4-complete` | Disk beaming + photon ring confirmed |
| Gate 5 | `iteration-5-gate-5` | Vulkan running, zero validation errors |
| Gate 6 | `iteration-5-complete` | CPU vs GPU L2 diff < 0.01 |
| Gate 8 | `iteration-6-complete` | Sgr A* ring 41.7–55.7 μas |
| Presets | `iteration-7-complete` | All four presets working |

**Tag command (run after every gate passes):**
```bash
git add -A
git commit -m "test: Gate N — [description of what passed]"
git tag -a iteration-N-complete -m "Iteration N complete: [one-line summary]"
```

Then screenshot `git log --oneline --decorate -5` and embed it in the write-up as evidence of version control practice. This earns marks under D9 (structure) and E8 (write-up quality — substantiated with evidence).

---

## Mark Scheme Summary

| Section | Marks | Key verbs for Very High band |
|---|---|---|
| Analysis | 10 | IDENTIFY, DESCRIBE, EXPLAIN, JUSTIFY |
| Design | 15 | EXPLAIN, JUSTIFY, DESCRIBE IN DETAIL |
| Iterative Development | 15 | EXPLAIN what was done and WHY |
| Iteration Testing | 10 | Evidence THROUGHOUT EACH STAGE |
| Post-Development Testing | 5 | Annotated evidence, function + robustness + usability |
| Evaluation | 15 | EXPLAIN how evidence shows criteria met/partially/not met |

---

---

# ANALYSIS SECTION
*(Write once before any iterations begin. This is the foundation of the whole write-up.)*

---

## Problem Identification and Summary of Solution

**What to write:**  
Open with a clear statement of the problem: modern physics education cannot convey the non-intuitive dynamics of curved spacetime and black hole optics through static diagrams or textbook equations. Existing interactive simulators either use physically incorrect approximations or run too slowly for classroom use.

Summarise the solution: a dual-engine C++ application running on modern Vulkan 1.3 that correctly implements Carter-separated Kerr geodesics (Bardeen, Press & Teukolsky 1972) for real-time ray-tracing of photon paths around a spinning black hole, paired with a Newtonian N-body gravity sandbox. The solution is designed to reproduce the photon-ring angular diameter of Sagittarius A* within the EHT-published measurement range of 48.7 ± 7 μas.

State the programming language (C++20), rendering API (Vulkan 1.3), build system (CMake 3.25 + Ninja + vcpkg manifest mode), and target platform (Windows, MSVC).

---

## A1 — Stakeholders

**What to write:**  
Produce a table with at least three real stakeholders. For each, include:
- Name and description (who they are, their level of technical background)
- Role (e.g., Quality Assurance / Tester, Content Setter, Casual User)
- Stakeholder needs — what they will use the solution for, and **why this solution is appropriate to their specific needs** (this must come from an actual interview or discussion with the real person)

**Required depth for Very High:**  
IDENTIFY suitable stakeholders. DESCRIBE them. EXPLAIN how they will make use of the solution. EXPLAIN why it is appropriate to their needs.

**Suggested stakeholders for this project:**
1. A physics or computer science teacher — will use the simulation to show students real Kerr black hole optics; requires scientific accuracy (conserved quantities, correct shadow shape) and a real-time frame rate.
2. A university student studying general relativity — will use the ImGui parameter sliders to explore how changing spin `a*` and inclination angle affects the shadow shape; requires the Boyer-Lindquist parameter display.
3. A casual user / GCSE-level student — will use the gravity sandbox mode to explore N-body orbital mechanics; requires an intuitive GUI and clear visual feedback.

> SCREENSHOT: After interviewing each stakeholder, take a screenshot of any notes or responses you received from them (a photo of written notes or a screenshot of an email reply is fine).

---

## A2 — Research

**What to write:**  
Research at least two existing similar solutions in depth. For each, include a labelled screenshot of the solution running (Figure 1, Figure 2, etc.), describe its features in detail, and then explicitly state which features you will incorporate into your solution and justify why.

**Suggested existing solutions to research:**

1. **kavan010/black_hole (GitHub)** — existing OpenGL Schwarzschild ray marcher. Features: GLSL compute shader dispatching, UBO camera matrices, real-time rendering. Limitations: Schwarzschild only (no Kerr), no conserved-quantity validation, no disk physics. Justify why your solution uses Kerr geodesics instead.

2. **Event Horizon Telescope 2022 Sgr A* results** — published shadow diameter 48.7 ± 7 μas, photon ring 51.8 ± 2.3 μas. Features: provides ground-truth calibration target. Justify why your simulation must match this.

3. **Starless (rantonels)** — Python-based Kerr ray tracer. Features: correct Carter equations, accurate shadow shape. Limitations: CPU-only, far too slow for real-time. Justify Vulkan compute shaders as the solution.

**For each feature you identify from research:**  
State whether you WILL include it and why, or will NOT include it and why (if not including, it becomes a limitation — see A4).

> SCREENSHOT: Take a screenshot of each existing solution running, or of the relevant paper/website, with labels pointing to the specific features you are discussing.

**Harvard references** (provide at the bottom of A2 and in the References section):
- Bardeen, J.M., Press, W.H. and Teukolsky, S.A. (1972) 'Rotating black holes: locally nonrotating frames, energy extraction, and scalar synchrotron radiation', *The Astrophysical Journal*, 178, pp. 347–370.
- Event Horizon Telescope Collaboration (2022) 'First Sagittarius A* Event Horizon Telescope results. I. The shadow of the supermassive black hole in the center of the Milky Way', *The Astrophysical Journal Letters*, 930(2), p. L12.
- Novikov, I.D. and Thorne, K.S. (1973) 'Astrophysics of black holes', in DeWitt, C. and DeWitt, B. (eds) *Black Holes*. New York: Gordon and Breach, pp. 343–450.

---

## A3 — Essential Features

**What to write:**  
List the essential functional features of the solution. Each feature must be a specific, measurable piece of functionality. IDENTIFY and DESCRIBE each feature, then EXPLAIN why it is essential.

**Minimum features to include:**

1. **Kerr geodesic integrator** — computes null geodesic paths using the four Carter-separated first-order ODEs in Boyer-Lindquist coordinates. Essential because Schwarzschild approximations do not produce physically accurate shadow shapes for spinning black holes.

2. **Conserved-quantity validation** — the integrator must maintain energy E, angular momentum L_z, and Carter's constant Q to within 1×10⁻⁶ relative error per geodesic. Essential to verify the physics is correct (without this there is no way to know the render is accurate).

3. **CPU reference ray tracer** — a CPU-side tracer that produces a reference image before any GPU work. Essential because it allows pixel-level comparison to validate the GPU shader.

4. **Vulkan compute shader renderer** — dispatches the geodesic integrator in parallel across all pixels at real-time frame rates. Essential for the interactive classroom use case (stakeholder requirement).

5. **Novikov-Thorne disk model** — accretion disk with temperature-dependent colour profile, inner edge at r_ISCO(a*), and relativistic Doppler beaming. Essential for physical accuracy and visual realism.

6. **Sgr A* calibration mode** — uses M = 4.297×10⁶ M☉ and d = 8,277 pc to render the shadow at the correct angular scale, validated against EHT 2022 photon ring diameter 48.7 ± 7 μas.

7. **ImGui parameter GUI** — live sliders for spin a*, inclination, mass, exposure, disk inner/outer radius. Essential for interactive stakeholder use.

8. **N-body gravity sandbox** — Velocity Verlet integration with inelastic merging and Roche limit tidal destruction. Essential for the gravity simulation mode (second application state).

9. **State machine** — transitions between MENU → SIM_BLACKHOLE and MENU → SIM_GRAVITY. Essential for switching between the two simulation modes.

---

## A4 — Limitations of Proposed Solution

**What to write:**  
IDENTIFY, EXPLAIN, and JUSTIFY what the solution will NOT do. For each limitation, justify why it is excluded — not just "not enough time" but a reasoned technical or scope argument.

**Suggested limitations:**

1. **No general relativistic magnetohydrodynamics (GRMHD)** — the accretion disk uses the Novikov-Thorne thin-disk approximation, not a full GRMHD simulation as used by EHT. Justification: GRMHD requires supercomputer-scale computation and is not feasible in real-time.

2. **No polarimetry rendering** — the simulation does not compute the polarisation angle of photons as they traverse the Faraday rotation region of the disk. Justification: polarimetry data would require synchrotron emission modelling which is beyond scope.

3. **No frame-dragging visualisation for the N-body sandbox** — the gravity sandbox uses Newtonian mechanics, not post-Newtonian corrections. Justification: Newtonian gravity is accurate enough for the pedagogical purpose of the sandbox, and mixing GR and Newtonian forces would be physically misleading.

4. **Maximum 128 bodies in the N-body sandbox** — the SSBO is sized for 128 GPUObjectData entries. Justification: 128 bodies is sufficient for all pedagogically meaningful orbital configurations and prevents GPU memory exhaustion.

---

## A5 — Computational Approach and Methods

**What to write:**  
Two parts:

**Part 1 — Why a computer-based solution is appropriate:**  
The problem involves integrating systems of four coupled first-order ordinary differential equations simultaneously for thousands of pixels in parallel. A paper-based or physical-model approach cannot do this. Real-time parallelism requires a GPU with thousands of cores. The result (a rendered image showing the photon ring) cannot be produced by any non-computational means.

**Part 2 — Which computational methods apply to which features:**  
For each feature from A3, identify and justify the computational method(s) used:

| Feature | Computational methods |
|---|---|
| Kerr geodesic integrator | Numerical methods (RK4 / RK45 adaptive), concurrent thinking (all pixels in parallel), decomposition (separating E, L_z, Q into four ODEs) |
| Conserved-quantity validation | Testing / abstraction (checking mathematical invariants without knowing the trajectory a priori) |
| CPU reference tracer | Thinking ahead (generating ground truth before GPU work), abstraction (camera-to-ray transformation) |
| Vulkan compute shader | Concurrency (thousands of SPIR-V threads), abstraction (bindless descriptors hide per-draw binding) |
| Novikov-Thorne disk | Pattern recognition (temperature falls off as a known power law with radius), visualisation |
| ImGui GUI | Abstraction (parameter struct decoupled from rendering), reusable components (Dear ImGui library) |
| N-body sandbox | Numerical methods (Velocity Verlet), decomposition (position update / velocity update / collision detection as separate steps) |
| State machine | Abstraction (game state as an enum), thinking ahead (pre-loading next state to minimise transition latency) |

---

## A6 — Success Criteria

**What to write:**  
At least 8 measurable, justified success criteria that are NOT about usability. Write them from the user's perspective. Each must be specific enough to test. Include some ambitious criteria that might not be fully met (needed for E4).

**Success criteria:**

1. **SC1 — Shadow correctness (Schwarzschild):** For spin a* = 0, the rendered shadow must be a perfect circle with angular radius within 2% of the analytic formula `θ = 3√3 M/r_obs`. *Justification: this is the fundamental test of whether the geodesic integrator is solving the correct equations.*

2. **SC2 — Shadow correctness (Kerr):** For spin a* = 0.94, the rendered shadow must be asymmetric — flattened on the prograde side and extended on the retrograde side — consistent with the published Bardeen (1973) shadow boundary. *Justification: the key scientific feature that distinguishes this from all Schwarzschild approximations.*

3. **SC3 — Conserved quantity drift:** Along any null geodesic, the relative drift in E, L_z, and Q must remain below 1×10⁻⁶ after 100 integration steps. *Justification: this is the only way to programmatically verify that the physics is correct.*

4. **SC4 — Sgr A* angular diameter:** Rendering Sagittarius A* with M = 4.297×10⁶ M☉, d = 8,277 pc, a* ≈ 0.94, must produce a photon ring angular diameter in the range 41.7–55.7 μas (EHT 2022, 48.7 ± 7 μas). *Justification: this is the only real-world ground truth for a Kerr black hole shadow.*

5. **SC5 — Real-time frame rate:** The Vulkan compute shader must render the black hole scene at ≥ 60 FPS at 1920 × 1080 on the development GPU. *Justification: stakeholder (teacher) requires real-time interaction for classroom use.*

6. **SC6 — Doppler beaming ratio:** The approaching side of the accretion disk must be at least 4× brighter than the receding side. *Justification: relativistic beaming is a fundamental observational signature of accreting black holes and must be visible for the simulation to be scientifically meaningful.*

7. **SC7 — ISCO inner disk edge:** The inner edge of the accretion disk must be at r_ISCO(a*), not at a hardcoded constant. For a* = 0 this must be 6M; for a* = 0.94 it must be substantially less than 6M. *Justification: the ISCO radius is a measurable prediction of GR and must be correct.*

8. **SC8 — Velocity Verlet energy stability:** In a closed two-body orbit, the total mechanical energy E = K + U must not drift by more than 0.05% over 60 minutes of simulation. *Justification: stakeholder requires orbits that look stable and realistic, not slowly decaying spirals.*

9. **SC9 — Inelastic collision merging:** When two bodies collide, total momentum must be conserved to within 0.001%. *Justification: momentum conservation is a fundamental physical law; a simulator that violates it would give students a wrong understanding of orbital mechanics.*

10. **SC10 — Roche limit tidal disruption:** Spawning a moon at a distance d < 2.44 R_p (m_p/m_m)^(1/3) from a planet must trigger tidal destruction within one simulation frame. *Justification: the Roche limit is a key concept in orbital mechanics and is explicitly required by one stakeholder.*

11. **SC11 — JSON save/load:** Saving and re-loading a simulation state must restore all body positions, velocities, and masses exactly. *Justification: users must be able to set up interesting scenarios and return to them.*

12. **SC12 — Zero Vulkan validation errors:** At no point during any render must the Vulkan validation layer report any errors or warnings. *Justification: validation errors indicate undefined behaviour that may crash the application or produce incorrect renders on different hardware.*

13. **SC13 — Visual shader presets:** The application must provide at least four named visual presets selectable at runtime via the ImGui interface. Each preset must produce a visually distinct render — different dominant colour, exposure, and glow characteristics — without any modification to the underlying geodesic or disk physics. Switching preset must take effect within one rendered frame. *Justification: different audiences require different visual styles — a researcher needs the physically motivated EHT colour map; a general-audience presentation benefits from the clean cinematic look from the Interstellar film; multiple presets also prove that the physics and shading layers are fully decoupled, which is a core architectural goal of the rewrite.*

---

## A7 — Requirements

**What to write:**  
IDENTIFY and JUSTIFY all hardware and software requirements for yourself (the developer) and for end users.

**Developer requirements:**
- Windows 10/11, MSVC 2022 (C++20 support required)
- GPU: Vulkan 1.3 capable (NVIDIA RTX or AMD RDNA 2+) — required for dynamic rendering extension
- CMake 3.25+, Ninja build system — required for CMakePresets.json workflow
- vcpkg in manifest mode — for reproducible dependency resolution
- Git — version control; justification: enables rollback to any previous iteration if a change breaks the build
- RenderDoc — GPU frame debugger; justification: required to verify dynamic rendering path and bindless descriptors (Gate 5)
- Screencastify — Chrome extension for test video recording (required by OCR)

**End user requirements:**
- Windows 10/11
- GPU with Vulkan 1.3 support (minimum: NVIDIA GTX 1060 or AMD RX 580)
- 8 GB RAM minimum (simulation with 128 bodies requires ~1.5 GB headroom)
- No internet connection required (standalone executable)

**Justify IDE choice:** Visual Studio Code with CMake Tools extension is used because it integrates with CMakePresets.json, provides IntelliSense for GLSL shader files, and is available on both the development machine and college computers.

**Version control strategy:** Git with a remote repository. Justification: if the laptop is lost or damaged, no code is lost; any broken iteration can be reverted with `git checkout`; each phase gate can be tagged as a release.

---

---

---

*Iterations 2–7 (design, development, testing, review) are documented directly in this file as each phase is completed. This analysis section is the foundation they build on.*

---
---

# ITERATION 1 — Physics Core & CPU Schwarzschild Reference Tracer
*(the project specification Phases 0–1. Version-control gates: `iteration-1-gate-0` = all Catch2 physics tests green; `iteration-1-complete` = CPU Schwarzschild shadow image saved and validated. Note: the gate-naming table earlier in this file uses `iteration-2-*`; the implemented tags follow the project specification §6 — these should be reconciled in a later pass.)*

The single most important failure identified in the review of the existing RSE project (Iteration 1 review document) was that **its physics was never validated**: the geodesic shader used a Newtonian-style angular-momentum approximation (`acc = -1.5·rs·h²·pos / r⁵`) with no conserved quantities, so there was no way to know whether a render was correct or merely "accidentally pretty." This iteration directly addresses that failure. Before any GPU code, it builds a **pure-CPU physics library that is unit-tested against analytic general-relativity results**, and a slow but correct **CPU reference ray tracer** that becomes the ground truth for all later GPU work.

## 1. Design

### 1.1 Architectural decision — separate the physics from the renderer
The old project entangled physics with OpenGL calls inside `main.cpp`, so the equations could never be tested in isolation. The rebuild is therefore split into a dependency-free static library, `kerr_physics`, that knows nothing about graphics, plus thin executables (tests and a render tool) that link against it. This is justified by the testing requirement (SC3): mathematical invariants such as Carter's constant can only be asserted automatically if the integrator is callable from a unit-test `main()` without spinning up a window or a GPU.

The library is built with the modern toolchain mandated in the project specification §1: **C++20**, **CMake + Ninja + `CMakePresets.json`**, **vcpkg manifest mode** (`catch2`, `glm`, `stb`), compiled with **MSVC 19.44**. Dependencies are declared once in `vcpkg.json` and resolved reproducibly, replacing the old project's fragile `FetchContent` with `CMAKE_TLS_VERIFY OFF`.

### 1.2 Coordinates and units
All physics is done in **geometric units (G = c = 1)** with the black-hole mass normalised to **M = 1**, so every length is measured in gravitational radii. This makes the analytic check values exact and dimensionless (horizon at r = 2, photon sphere at r = 3, ISCO at r = 6), which is ideal for unit testing. Spin is stored as `a = J/M`; the dimensionless spin is `a* = a/M ∈ [0,1)`. The metric uses **Boyer–Lindquist coordinates** (the project specification §3).

### 1.3 `KerrMetric` (Fig. 1)
`KerrMetric` exposes the metric building blocks Σ = r²+a²cos²θ, Δ = r²−2Mr+a², A = (r²+a²)²−a²Δsin²θ, and the **characteristic radii** that the whole project is calibrated against:

| Quantity | Implemented formula | Schwarzschild value (a*=0) |
|---|---|---|
| Outer horizon r₊ | M + √(M²−a²) | 2M |
| Ergosphere r_E(θ) | M + √(M²−a²cos²θ) | 2M |
| Photon sphere (prograde) | 2M{1+cos[⅔·arccos(−a/M)]} | 3M |
| Photon sphere (retrograde) | 2M{1+cos[⅔·arccos(+a/M)]} | 3M |
| ISCO (pro / retro) | BPT72 Z₁,Z₂ form, 3+Z₂∓√[(3−Z₁)(3+Z₁+2Z₂)] | 6M |

The prograde/retrograde split is deliberate and is the headline test of whether spin actually does anything (the project specification §3): at a*=0 both must collapse to the Schwarzschild value, and for a*>0 the co-rotating orbit must sit closer to the hole than the counter-rotating one.

### 1.4 Geodesic integrator — the integrator health-check
Rather than the old project's ad-hoc acceleration, the integrator evolves the **Carter-separated super-Hamiltonian** `H = ½ gᵘᵛ pᵤ pᵥ`, which equals 0 for photons. Energy `E = −p_t` and axial angular momentum `L_z = p_φ` are constants of motion (the metric is independent of t and φ), so only the four variables (r, θ, p_r, p_θ) are integrated, with `t` and `φ` carried along. The equations of motion are Hamilton's equations; the radial/polar forces −∂H/∂r and −∂H/∂θ are evaluated by **central finite differences** of the analytic inverse metric (chosen over hand-derived analytic derivatives to eliminate a whole class of algebra bugs, with the difference step tuned so its error is far below the 1×10⁻⁶ conservation tolerance). Integration is **classic RK4**; θ is clamped 1×10⁻⁸ away from the poles to avoid the Boyer–Lindquist coordinate singularity (the project specification §9).

Crucially, after every step the code **recomputes E, L_z, Q and H from the live state** so they can be asserted constant. This is the mechanism the old project lacked entirely (Fig. 2).

### 1.5 CPU reference ray tracer (Fig. 3)
The tracer performs **backward ray tracing**: a photon is launched from the observer along each image-plane direction and integrated until it (a) falls through the horizon → *shadow* (black), (b) escapes to large r → *sky*, or (c) crosses the equatorial plane within the disk annulus → *disk*. Image-plane coordinates are the celestial **impact parameters (α, β)** of Bardeen (1973): with the observer at polar angle θ_obs, `L_z = −α·sinθ_obs` and `p_θ = β`, and p_r is then solved from the null condition H = 0. For Schwarzschild the total impact parameter `b = √(α²+β²)` is conserved and fully determines capture: the photon is swallowed **iff b < b_c = 3√3 M ≈ 5.196 M**. This analytic value is the project's first quantitative success criterion (SC1).

Two numerical-design choices matter:
* **Adaptive step** `dλ = clamp(0.04·(r−r₊), 0.003, 1.5)` — tiny near the hole where the path bends sharply, large in the weak field where the photon is nearly straight; this keeps the step count (and render time) low without missing turning points.
* **Supersampling** — see §3, added after the first render revealed aliasing.

Pseudocode for a single ray:
```
trace_ray(metric, α, β, camera, disk):
    L_z ← −α·sin(θ_obs);  geo ← KerrGeodesic(metric, E=1, L_z)
    s   ← geo.make_null_state(r_obs, θ_obs, p_θ=β, ingoing)
    repeat up to MAX_STEPS:
        prev ← s;  s ← RK4_step(s, adaptive_dλ(s.r))
        if disk enabled and (prev.θ−π/2) and (s.θ−π/2) straddle 0:
            r_cross ← interpolate r at θ=π/2
            if r_inner ≤ r_cross ≤ r_outer: return DISK(r_cross)
        if s.r ≤ r₊+ε:            return HORIZON
        if s.r ≥ r_escape and rising: return ESCAPED
```

## 2. Development

### 2.1 Build environment
Standing up the toolchain was the first real obstacle and is documented because it shaped later decisions. The development machine had **no compiler, ninja, or vcpkg on the PATH**: MSVC exists only inside the VS 2022 *Build Tools* and is usable only after sourcing `vcvars64.bat`; `ninja` had to be recovered from the vcpkg cache; and the network is restricted (consistent with the old project's notes), though vcpkg succeeded in fetching and caching Catch2 3.14.0, glm 1.0.3 and stb. A small wrapper batch (`vcvars64 → cd project → run`) drives all `cmake`/`ctest` commands. Configuration output is shown in **Fig. 4**.

### 2.2 Implementation order and version control
Development followed the project specification's phase order and committed at every logical step (Fig. 9): scaffold → build system → physics headers → implementations → tests → reference tracer. Headers were written before their `.cpp` files so the interface design could be reviewed first (Figs. 1–3).

### 2.3 A real compile error (Iterative Development evidence)
The first build of the render tool **failed to compile** (Fig. 5): `error C3083: 'numbers'` / `'pi_v' is not a member of 'std'`. The cause was a missing `#include <numbers>` in `render_reference_schwarzschild.cpp` — `std::numbers::pi_v<double>` is a C++20 facility that requires its header explicitly. The fix was a one-line include; the subsequent build linked cleanly (Fig. 6). This error is retained as evidence because it is exactly the kind of issue the iterative process is meant to surface and resolve quickly.

## 3. Iteration

The iteration proceeded as two validated steps, each ending at a tagged gate:

**Step A → `iteration-1-gate-0`.** The metric and geodesic integrator were implemented and the three Catch2 suites written. `ctest` came back green on the first run after the physics compiled (Fig. 7 shows the later 4-suite run). The conserved-quantity suite passing is the decisive result: it proves the integrator is solving the *correct* equations, which the old project could never demonstrate.

**Step B → `iteration-1-complete`.** The CPU reference tracer was added and a 300×300 Schwarzschild image rendered. The **first render exposed a defect**: a dashed vertical seam down the `L_z ≈ 0` meridian. Investigation showed this is not a bug but **under-sampled real physics** — along that meridian the azimuthal motion vanishes, so the disk is imaged repeatedly in nested higher-order lensed rings that are finer than one pixel. The response was to add **n×n supersampling** to `render_image()` and re-render at 2× (Fig. 8). This is a textbook iteration: render → observe artefact → diagnose → improve → re-render. The seam is reduced but not eliminated at 2×; it is documented as a known limitation, with adaptive supersampling near the photon ring noted as future work.

The measured shadow edge was identical before and after supersampling — **b_c = 5.1962 M against the analytic 3√3 = 5.1962 M, a 0.000 % error** (Fig. 6) — confirming the visual change was cosmetic and the physics unchanged.

## 4. Testing

Testing in this iteration is **automated and analytic**: every assertion compares the code against a closed-form GR result, so a regression in any later phase will trip a red test immediately. All suites run under `ctest --preset debug`.

| # | Suite / case | What it checks | Expected | Result |
|---|---|---|---|---|
| 1 | `test_schwarzschild_limit` | a*=0 collapses to Schwarzschild | r₊=2M, r_E=2M, r_ph=3M (pro=retro), r_ISCO=6M | **Pass** |
| 2 | `test_key_radii` | spin breaks pro/retro symmetry (a*=0.5, 0.94) | r_ph,pro < r_ph,retro; r_ISCO,pro < r_ISCO,retro; pro ISCO → r₊ as a*→1 | **Pass** |
| 3 | `test_conserved_quantities` (SC3) | E, L_z, Q & H along a null geodesic, 100 RK4 steps | relative drift < 1×10⁻⁶, \|H\| ~ 0 | **Pass** |
| 4 | `test_shadow_size` (SC1) | CPU tracer shadow edge & circularity | b_c = 3√3 M within 2%; b_c equal along α, β, diagonal; capture inside / escape outside | **Pass** |

`ctest` reports **100 % tests passed, 0 failed out of 4** (Fig. 7). The reference render reports the shadow radius directly: **b_c = 5.1962 M (0.000 % error)** (Fig. 6). The rendered image (Fig. 8) shows the three features expected of a correct Schwarzschild render with an inclined thin disk: a circular black shadow, the disk's near edge crossing in front, and the disk's far side **lensed up and over the top** of the shadow into a bright ring — gravitational lensing that emerges automatically from integrating real geodesics, never hard-coded.

**Success criteria met this iteration:** SC1 (Schwarzschild shadow circle, ≤2 %) ✓; SC3 (conserved-quantity drift < 1×10⁻⁶) ✓. SC2/SC4 (Kerr shadow asymmetry, Sgr A* calibration) are deferred to later iterations as planned.

### Figures
1. `screenshots/iteration_1/iter1_metric_header.png` — `KerrMetric` interface.
2. `screenshots/iteration_1/iter1_geodesics_header.png` — geodesic integrator interface (conserved-quantity accessors).
3. `screenshots/iteration_1/iter1_raytrace_header.png` — CPU reference tracer interface.
4. `screenshots/iteration_1/iter1_cmake_config.png` — successful CMake/vcpkg configuration.
5. `screenshots/iteration_1/iter1_phase1_err_before.png` — compile error (missing `<numbers>`).
6. `screenshots/iteration_1/iter1_phase1_render_output.png` — fixed build + measured b_c = 5.1962 M.
7. `screenshots/iteration_1/iter1_ctest_phase1.png` — all 4 Catch2 suites passing.
8. `screenshots/iteration_1/iter1_schwarzschild_shadow.png` — CPU reference image (`validation/reference_images/schwarzschild_shadow_cpu.png`).
9. `screenshots/iteration_1/iter1_gate0_tag.png` — git history with the `iteration-1-gate-0` tag.

---
---

# ITERATION 2 — Kerr (Spinning) Black Hole Geodesics
*(the project specification Phase 2. Gate `iteration-2-gate-2` = Kerr θ-oscillation confirmed and Carter's Q conserved. The remainder of this iteration — the lopsided Kerr CPU shadow render, Gate 3 / `iteration-2-complete` — is still ahead.)*

The review's most fundamental physics complaint was that the old project **faked spin**: it bolted a weak-field Lense-Thirring term (`frameDrag = spin·rs²·2.5/r⁴ · (spinVec × pos)`) onto a Newtonian acceleration. That term is only valid far from the hole, so the spinning shadow it produced was not physically meaningful. This iteration proves the rebuilt integrator carries **genuine Kerr physics** all the way down to the photon sphere.

## 1. Design
A key architectural pay-off surfaces here: the geodesic integrator built in Iteration 1 is **already full Kerr**. The super-Hamiltonian `H = ½ gᵘᵛ pᵤ pᵥ` uses the complete inverse Kerr metric, including the off-diagonal `g_tφ` term that *is* frame dragging, and `KerrMetric` already carries the spin `a`. So Phase 2 needs **no new physics code** — it is validation-led, designed to prove the spin terms are physically active rather than inert.

Three independent, observable spin signatures were chosen as tests:
1. **Carter's Q conserved at high spin (a\*=0.9)** — the integrator must stay healthy in the strong-spin regime, not just at a*=0.5.
2. **Frame dragging (Lense-Thirring)** — a photon with *zero* axial angular momentum (L_z=0) must still be swept around in azimuth φ by a spinning hole, and must *not* be for a non-spinning one. This isolates the `g_tφ` term.
3. **Latitudinal (θ) oscillation** — a non-equatorial photon must bounce between Carter turning points in θ. To make this clean and sustained, the test uses a **spherical photon orbit** (constant r) with the analytic constants L(r_s), Q(r_s) of Teo (2003).

## 2. Development
A single new suite, `test_kerr_geodesics.cpp` (Fig. 12), was added — no changes to the physics library, which is itself strong evidence that the Iteration 1 design generalised correctly to spin. The frame-dragging test drops an L_z=0, Q=0 photon from r=15 M on the equator and integrates to the horizon, comparing the accumulated Δφ at a\*=0 against a\*=0.9. The θ-test places a photon on the spherical orbit at r_s = 2.5 M (which lies between the prograde 1.56 M and retrograde 3.91 M photon spheres for a\*=0.9, so the orbit exists), starting on the equator with p_θ = √Q, and counts the reversals of p_θ.

## 3. Iteration
All three checks passed on the first run. The most striking diagnostic was the spherical orbit holding its radius to **[2.49995, 2.5] M** through a full latitude swing — confirming that the `make_null_state` null-condition solver placed the photon onto the orbit essentially exactly, and that RK4 + the finite-difference forces hold an *unstable* orbit cleanly over the test span. No tuning was needed.

## 4. Testing (Gate 2)

| Case | What it checks | Expected | Measured | Result |
|---|---|---|---|---|
| Q conserved (a\*=0.9) | \|ΔQ\|/Q and \|H\| over 100 RK4 steps | < 1×10⁻⁶ | within tol | **Pass** |
| Frame dragging | Δφ for L_z=0 photon, a\*=0 vs 0.9 | a\*=0 → 0; a\*=0.9 → large | **0 vs 2.791 rad (~160°)** | **Pass** |
| θ-oscillation | p_θ turning points; latitude range; r constant; Q held | ≥2 turns, real swing, r≈const | **2 turns, θ∈[0.045, 3.097], r∈[2.49995, 2.5]** | **Pass** |

`ctest` reports **5/5 suites passing** (Fig. 10). The frame-dragging measurement (Fig. 11) is the headline result: a photon with no angular momentum of its own is dragged **2.791 radians** around an a\*=0.9 hole purely by the rotation of spacetime, versus **exactly 0** with no spin — the defining Kerr signature, computed from the real equations rather than a far-field approximation.

**Success criteria progress:** SC3 (conserved-quantity drift) re-confirmed at high spin. SC2 (visibly lopsided Kerr *shadow*) is the next step — the geodesics are now proven correct, so the remaining work is to feed them through the CPU ray tracer at a\*≠0 and render the flattened shadow (Gate 3).

### Figures
10. `screenshots/iteration_2/iter2_ctest_gate2.png` — 5 Catch2 suites passing.
11. `screenshots/iteration_2/iter2_framedrag_theta.png` — measured frame dragging (2.791 rad) and θ-oscillation.
12. `screenshots/iteration_2/iter2_kerr_test.png` — the Kerr geodesics test suite.

---
---

# ITERATION 3 — Lopsided Kerr Shadow & Relativistic Disk Emission (Phases 3–4)
*(Closes two gates: `iteration-2-complete` = the spinning shadow renders lopsided (Phase 3); `iteration-3-complete` = the accretion disk shows relativistic beaming, the lensed far side, and an ISCO inner edge (Phase 4). Both are demonstrated by the single render in Fig. 14.)*

With the Kerr geodesics proven correct (Iteration 2), this iteration finally turns them into a picture and adds the physics that makes an accreting black hole *look* the way it does. It produces the first image that is impossible for any of the old project's approximations to reproduce correctly: a spinning black hole with a Doppler-beamed disk.

## 1. Design
**Phase 3 (lopsided shadow).** No new tracing code is needed — the CPU reference tracer from Iteration 1 already launches rays through the full Kerr metric. Rendering at a\* = 0.9 instead of 0 is enough to produce the asymmetric shadow, because frame dragging shifts the capture cross-section sideways. The shape is quantified with `critical_impact_parameter()` measured along opposite image directions.

**Phase 4 (disk emission).** A new module, `disk.hpp/.cpp` (Fig. 13), adds the emission physics for a thin, optically thick, prograde equatorial disk (the project specification §10):
- **Redshift factor** `g = √(r³ − 3r² + 2a·r^{3/2}) / ((r^{3/2}+a) − b)`, combining gravitational time dilation with the orbital Doppler shift for a photon of impact parameter `b = L_z/E`.
- **Relativistic beaming** `I_obs ∝ g⁴` — the single most visually dominant effect.
- **Temperature** `T ∝ (r_inner/r)^{3/4}` with the inner edge pinned to `r_ISCO(a*)`.
- **Blackbody colour** ramp (red → orange/white → blue), driven by the *observed* colour temperature `g·T`, so the approaching side shifts blue and the receding side red.

These feed the tracer's disk-hit branch: each disk pixel is shaded by `blackbody_color(g·T)` scaled by a Reinhard-tone-mapped `g⁴·T⁴` brightness.

## 2. Development
`disk.cpp` was implemented and unit-tested in isolation first (Stage 1), then wired into `render_image()` (Stage 2), and a dedicated tool `render_kerr_disk.cpp` renders a\*=0.9 at 80° inclination. The disk inner edge is set programmatically to `KerrMetric::r_isco_prograde()` (= 2.321 M at a\*=0.9), never a constant.

## 3. Iteration
The first render (correct physics, default exposure) came out **almost entirely black** — only the bright approaching inner edge was visible. Diagnosis: the `g⁴` beaming spans an enormous dynamic range, so a linear exposure crushed everything else to zero. The fix was a tone-mapping change only (raise exposure, add a 1/2.2 gamma curve); the physics and the measured numbers were unchanged. The second render (Fig. 14) shows the full disk with the asymmetry intact — a clean example of separating a *display* problem from a *physics* problem.

## 4. Testing (Gates 3 & 4)

| Case | What it checks | Expected | Measured | Result |
|---|---|---|---|---|
| Kerr shadow lopsided (SC2) | shadow edge, +α vs −α, a\*=0.9 | edges differ; a\*=0 equal | **+α = 6.832 M, −α = 2.844 M (Δ = 3.99 M)**; a\*=0 symmetric | **Pass** |
| Disk redshift | g approaching vs receding, r=6 | g_app > 1 > g_rec; baseline √(1−3/r) | as expected | **Pass** |
| Beaming asymmetry (SC6) | I_obs(app)/I_obs(rec) | ≥ 4× | **17.0×** | **Pass** |
| Temperature / ISCO edge (SC7) | T(r) falls; inner edge = r_ISCO | 6 M at a\*=0, < 6 M at high spin | inner edge **2.321 M** at a\*=0.9 | **Pass** |

`ctest` now reports **7/7 suites green** across Phases 0–4 (Fig. 17). The render (Fig. 14) visibly satisfies the Phase 4 gate: the approaching side of the disk is far brighter and bluer than the receding side, the far side of the disk is gravitationally lensed up and over the shadow, and the shadow itself is flattened on the prograde side.

**Success criteria met:** SC2 (lopsided Kerr shadow) ✓, SC6 (≥4× beaming asymmetry) ✓, SC7 (ISCO inner edge tracks a\*) ✓ — alongside SC1/SC3 from earlier. Remaining ambitious criteria (SC4 Sgr A* angular calibration, SC5 real-time GPU frame rate, SC12 zero Vulkan validation errors) belong to the GPU iterations.

*Known limitation:* a faint seam persists along the `L_z≈0` image meridian (under-sampled higher-order lensing); adaptive supersampling there is noted as future work.

### Figures
13. `screenshots/iteration_3/iter3_disk_header.png` — disk emission interface.
14. `screenshots/iteration_3/iter3_kerr_disk.png` — **the headline render**: a\*=0.9 lopsided shadow + Doppler-beamed disk.
15. `screenshots/iteration_3/iter3_kerr_render_output.png` — measured shadow asymmetry and ISCO inner edge.
16. `screenshots/iteration_3/iter3_disk_beaming.png` — beaming brightness ratio (17×).
17. `screenshots/iteration_3/iter3_ctest_gate4.png` — all 7 Catch2 suites passing.
