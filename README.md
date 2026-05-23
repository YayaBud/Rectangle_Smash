# RECTANGLE SMASH: ULTIMATE EDITION

A fast, punishing 2D top-down arena shooter built in C++ with SFML 2.6. Fight through escalating waves of geometric enemies, survive two boss encounters, and push your run as far as possible with perks, upgrades, and ship-specific abilities.

---

## GAMEPLAY OVERVIEW

You pilot a ship from the bottom of the screen, shooting upward at enemies that spawn from the top. Waves get progressively harder — more enemy types, faster projectiles, shields, and ghost variants. Every five waves triggers a boss fight. Survive long enough and you'll face Surt, the Geometry God, at wave 10.

Between waves you pick perks, spend Gears in the Shop, and invest in permanent upgrades in the Black Market. Die and start over — but your meta-upgrades carry.

---

## CONTROLS

| Action | Input |
|---|---|
| Move | `WASD` or Arrow Keys |
| Aim | Mouse |
| Primary Fire | Left Click (or Auto-Fire in Settings) |
| Secondary Weapon | Right Click |
| Dash | Space |
| Tactical Bomb | Left Shift |
| Pause | Escape |
| Return to Menu | `M` (while paused) |

---

## MECHANICS

**Movement & Combat**

- Velocity-based movement with configurable speed. Enable Mouse-Follow mode in Settings for an alternative control scheme.
- **Dash** (Space) grants full invincibility frames while active. Dash the moment a bullet would hit you to trigger a **Perfect Parry**, which grants extended invincibility.
- **Weapon Heat** — continuous firing builds heat. Overheating locks your guns until they cool. Enable Auto-Fire for convenience at the cost of a heat penalty.
- **Graze** — flying close to enemy bullets charges your ultimate meter and scores bonus points.
- **Tactical Bomb** — clears all enemy projectiles from the screen and deals 30 damage to every visible enemy. Ten-second cooldown.

**Secondary Weapons** (Right Click, costs energy)

Each ship class has a unique secondary:
- **Tank** — drops proximity mines that deal AoE damage and trigger on contact.
- **Speedster** — fires four homing missiles that track the nearest enemy.
- **Sniper** — releases a wide shotgun spread of twelve fast projectiles.

**Shield & Recovery**

A passive shield absorbs one hit before going on cooldown. Power-ups drop from enemies and offer health, fire rate, invincibility windows, and other temporary boosts.

**Combo System**

Kills chain into a combo multiplier. Chain fast enough and kill banners trigger — Triple Kill, Rampage, Unstoppable, Godlike. Combo resets on a short timer between kills.

---

## ENEMIES

| Type | Behavior |
|---|---|
| Yellow Zigzagger | Fast diagonal movement, low HP |
| Magenta Shooter | Stops and fires aimed bursts from wave 4+ |
| Red Homing | Large, slow, tracks the player |
| Green Swarm | Spawns in groups of 2–5, very low HP |
| Cyan Dodger | Actively evades your cursor (wave 6+) |

From wave 3 onward, enemies can spawn with **shields** that must be stripped before dealing HP damage. From wave 6, some enemies are **ghosts** — invisible until they materialize.

---

## BOSSES

**Boss 1 — The Rectangle Overlord** (waves 5, 15, 20…)

A two-phase fight. Phase 1 features radial bullet bursts and teleporting minion spawns. Phase 2 activates a screen-wide Mega-Beam. Defeating the boss triggers a slow-motion death cinematic.

**Boss 2 — Surt, the Geometry God** (wave 10)

A six-stage escalating encounter. Surt changes attack patterns, bullet density, and behavior at each health threshold. The final stage is a high-speed Rage Mode. This fight has its own beam attack, and both bosses respond to the Tactical Bomb.

---

## PROGRESSION

**During a Run**

- **Gears** drop from enemies and are spent in the Shop on weapon level, move speed, fire rate, dash cooldown, and max health upgrades (5 levels each).
- **Perks** are offered at the end of each wave — choose one of three randomly drawn cards. Examples include Bouncy Lasers, Glass Cannon, and Vampirism.
- **Companion Drones** can be unlocked mid-run and autonomously shoot nearby enemies.

**Permanent (Black Market)**

Gears spent here carry across deaths:
- Starting bonus HP
- Permanent speed boost
- Damage multiplier
- Starting Gear count

**Ship Classes**

Select before each run. Three archetypes with different stat spreads and secondary weapons (see above). Multiple skins are available per class.

---

## SETTINGS

Configurable in-game:
- Move speed (1–10)
- Fire speed (adjustable timer)
- Movement mode: WASD or Mouse-Follow
- Auto-Fire toggle
- Overheat toggle

---

## BUILD

**Requirements**
- Visual Studio 2022 with the "Desktop development with C++" workload
- SFML 2.6.x (headers and binaries included under `External/SFML/`)

**Steps**
1. Clone the repository.
2. Open `RectangleSmash.sln` in Visual Studio 2022.
3. Set platform to **x64** and configuration to **Release** (or Debug).
4. Press F5 to build and run.

Assets (textures, fonts) must be present in `RectangleSmash/` relative to the executable.

---

## PROJECT STRUCTURE

```
RectangleSmash/      — Main source directory
  game.cpp           — Master update/render loop, event polling
  game_init.cpp      — Resource loading, procedural audio generation
  game_update.cpp    — Per-frame logic for all 15+ game states
  game_render.cpp    — Rendering pipelines, UI, particles, glow effects
  game_spawn.cpp     — Enemy spawning, wave logic, score/combo helpers
  game_features.cpp  — Bombs, drones, secondary weapons
  boss.cpp           — Boss 1 state machine (phases, projectiles, beam)
  Boss2.cpp          — Boss 2 (Surt) six-stage state machine
assests/             — Textures and background music
fonts/               — TrueType and OpenType fonts
External/SFML/       — SFML 2.6 binaries and headers
```

**Tech stack:** C++17 · SFML 2.6 · Visual Studio 2022 · Win32 (debug logging)

Audio effects for explosions, dashes, and hits are generated procedurally via math functions at startup rather than loaded from files.
