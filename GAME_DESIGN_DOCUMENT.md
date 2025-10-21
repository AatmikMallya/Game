# Voxel Magic Combat Game - Design Document

**Version:** 2.0 - Focused MVP
**Date:** 2025-10-16
**Philosophy:** Perfect ONE thing before adding more

---

## Table of Contents

1. [Long-Term Vision](#1-long-term-vision)
2. [MVP Philosophy](#2-mvp-philosophy)
3. [MVP Scope - The Fireball Loop](#3-mvp-scope---the-fireball-loop)
4. [Core Gameplay Loop](#4-core-gameplay-loop)
5. [Fireball Spell Design](#5-fireball-spell-design)
6. [Enemy Design - The Slime](#6-enemy-design---the-slime)
7. [Game Feel & Polish](#7-game-feel--polish)
8. [Asset Requirements](#8-asset-requirements)
9. [Implementation Roadmap](#9-implementation-roadmap)
10. [Testing & Iteration](#10-testing--iteration)
11. [Success Criteria](#11-success-criteria)
12. [Post-MVP Vision](#12-post-mvp-vision)

---

## 1. Long-Term Vision

### 1.1 The End Goal

A deep, immersive first-person voxel magic combat game featuring:

**World:**
- Procedurally generated 3D worlds (caves, floating islands, dungeons, towers)
- Diverse biomes with unique voxel types and physics
- Dynamic terrain that evolves through gameplay
- Environmental hazards (lava lakes, quicksand, collapsing structures)

**Magic System:**
- 20-30 unique spells across multiple schools
- Deep simulation (fire spreads, water flows, ice melts, plants grow)
- Complex spell interactions and emergent combos
- Magic affects terrain permanently

**Enemies:**
- 15+ enemy types with unique behaviors
- Enemy factions that fight each other
- Boss encounters with phases and mechanics
- Enemies that interact with voxel world (dig, climb, destroy)

**Progression:**
- Roguelike runs (permadeath, meta-progression)
- Unlock new spells and abilities
- Build variety (different playstyles)
- Increasing difficulty and complexity

### 1.2 Core Pillars (Long-Term)

1. **Environmental Destruction** - Everything is destructible and persistent
2. **Emergent Simulation** - Systems interact in unpredictable ways
3. **Magical Mastery** - Deep spell system with high skill ceiling
4. **Spatial Combat** - 3D positioning and verticality matter
5. **Procedural Discovery** - Every run is unique

### 1.3 Inspirations

- **Noita** - Deep simulation, emergent chaos, permadeath runs
- **Teardown** - Satisfying voxel destruction
- **Spelunky** - Tight gameplay loop, procedural generation
- **Risk of Rain** - Escalating difficulty, build variety

---

## 2. MVP Philosophy

### 2.1 The One Thing Rule

**We are building ONE perfect 60-second combat loop.**

Not 5 spells that feel OK. Not 3 enemies that are buggy. ONE spell and ONE enemy that feel AMAZING.

### 2.2 Why This Approach?

**Learning Through Focus:**
- Master the spell → projectile → voxel destruction pipeline
- Perfect enemy AI before adding variety
- Nail game feel (screen shake, particles, sound) once
- Understand performance implications deeply
- Build reusable systems (then adding more is easy)

**Quality Over Quantity:**
- Players remember how good ONE thing feels
- Better to have 1 spell players love than 5 they ignore
- Polished demo > feature-complete mess
- Can scale up AFTER we know it works

**Faster Iteration:**
- Change one spell, test immediately
- Tune one enemy, see results
- Less code to debug
- Clearer success metrics

### 2.3 MVP Success = The Fireball Loop

```
See enemy
   ↓
Aim fireball
   ↓
Cast (satisfying sound, visual feedback)
   ↓
Projectile flies (smooth, readable)
   ↓
Impact (EXPLOSION, screen shake, voxels destroyed)
   ↓
Enemy dies (dramatic death, satisfying feedback)
   ↓
Player feels: "HELL YEAH, I want to do that again!"
```

If this 60-second loop feels great, we have a game.
If it doesn't, adding more content won't fix it.

---

## 3. MVP Scope - The Fireball Loop

### 3.1 What's In (Must Have)

**✅ Already Done:**
- Player movement (flying/walking)
- Health system (can take damage, die)
- Mana system (regenerates over time)
- Basic UI (health bar, mana bar)
- Voxel world (floating islands)
- Spell casting framework

**To Implement:**
1. **Fireball Spell**
   - Projectile that travels forward
   - Explodes on impact with voxels or enemies
   - Destroys voxels in radius
   - Creates lava at center (persists)
   - Deals damage to enemies in blast radius

2. **Slime Enemy**
   - Spawns at distance from player
   - Detects player, chases directly
   - Melee attack when in range
   - Takes damage from fireball
   - Dies with satisfying effect
   - Respawns (continuous pressure)

3. **Game Loop**
   - Enemy spawner (1 at a time initially)
   - Kill enemy → score point → spawn another
   - Get hit → lose health → die at 0 HP
   - Death screen with stats (kills, time)
   - Restart button

4. **Polish & Feel**
   - Screen shake on explosion
   - Fireball trail particles
   - Explosion particles
   - Impact sound (satisfying BOOM)
   - Fireball cast sound
   - Enemy death effect
   - Damage flash on enemy hit
   - Hit marker / crosshair feedback

### 3.2 What's Out (Explicitly Deferred)

**Not in MVP:**
- ❌ Multiple spells (ONLY fireball)
- ❌ Multiple enemy types (ONLY slime)
- ❌ Spell combos
- ❌ Enemy variety
- ❌ Different biomes
- ❌ Procedural generation
- ❌ Meta-progression / unlocks
- ❌ Boss fights
- ❌ Different game modes
- ❌ Achievements
- ❌ Tutorial
- ❌ Story/narrative
- ❌ Music (ambient sound OK, not required)
- ❌ Advanced AI (pathfinding, tactics)
- ❌ Spell upgrades
- ❌ Difficulty settings

**Why?**
Because if the fireball loop isn't fun, none of that matters.

### 3.3 MVP Feature Set Summary

| Feature | Status | Priority |
|---------|--------|----------|
| Player movement | ✅ Done | - |
| Health system | ✅ Done | - |
| Mana system | ✅ Done | - |
| Basic UI | ✅ Done | - |
| **Fireball projectile** | ⏳ TODO | **CRITICAL** |
| **Fireball explosion** | ⏳ TODO | **CRITICAL** |
| **Slime enemy** | ⏳ TODO | **CRITICAL** |
| **Enemy spawning** | ⏳ TODO | **CRITICAL** |
| **Combat feedback** | ⏳ TODO | **CRITICAL** |
| Game over screen | ⏳ TODO | High |
| Audio (SFX) | ⏳ TODO | High |
| Particle effects | ⏳ TODO | High |
| Screen shake | ⏳ TODO | Medium |

---

## 4. Core Gameplay Loop

### 4.1 The 60-Second Loop (One Enemy)

```
0:00 - Enemy spawns 20 units away
       Player spots enemy
       Player positions for clear shot

0:05 - Player aims at enemy
       Crosshair turns red (enemy in sights)

0:06 - Player casts fireball (left click)
       Satisfying "FWOOSH" sound
       Mana decreases (20 mana)
       Hands glow briefly

0:07 - Fireball projectile launches
       Smooth arc through air
       Trailing fire particles
       Whoosh sound as it travels

0:09 - Fireball hits slime
       EXPLOSION (visual, sound, shake)
       Voxels destroyed in radius
       Lava created at center
       Slime takes 50 damage
       Slime dies (only has 50 HP)

0:10 - Slime death effect
       Particle burst
       Death sound
       Body fades out
       "+1 KILL" appears on screen
       Score increments

0:12 - Another enemy spawns
       Cycle repeats
```

**Feel:** Smooth, responsive, powerful, satisfying

### 4.2 The 5-Minute Session

```
0:00 - Start: Full health (100), full mana (100)
       One enemy spawns

1:00 - Killed 10 enemies
       Player getting comfortable
       Mana management becoming natural

2:00 - Killed 20 enemies
       Terrain partially destroyed
       Player using destroyed terrain tactically

3:00 - Killed 30 enemies
       Getting harder (spawn rate increases?)
       Player in flow state

4:00 - Killed 40 enemies
       Player gets hit once (-20 HP)
       Heightened tension

5:00 - Player reaches 50 kills = VICTORY
       OR player dies = DEFEAT
       Show stats screen
       "Play Again" button
```

### 4.3 What Makes It Fun?

**Moment-to-Moment:**
- Aiming and prediction (projectile travel time)
- Timing the shot (mana cost, cooldown)
- Satisfying destruction (explosion, voxels fly)
- Power fantasy (one-shot kills)
- Rhythm (spawn, aim, shoot, explode, repeat)

**Session-Level:**
- Improvement over time (better aim, positioning)
- Flow state (lose track of time)
- High scores (beat your record)
- "One more run" compulsion
- Environmental changes (terrain destroyed)

**Why Players Return:**
- Skill improvement (get better at aiming)
- Consistency (can I get 50 kills every time?)
- Experimentation (what if I excavate pit first?)
- Satisfying core loop (explosion never gets old)

---

## 5. Fireball Spell Design

### 5.1 Spell Properties

**Basic Stats:**
- **Mana Cost:** 20 (can cast 5 times before need regen)
- **Cooldown:** 1.0 second (prevents spam, but not frustrating)
- **Projectile Speed:** 15 units/sec (slow enough to aim, fast enough to feel good)
- **Projectile Lifetime:** 5 seconds (despawn if miss)
- **Range:** ~75 units effective

**Damage:**
- **Direct Hit:** 50 damage (one-shot kill slime)
- **Explosion Radius:** 4 units (blast radius)
- **Explosion Damage:** 50 (same as direct, no falloff in MVP)
- **Player Damage:** YES (20 damage if caught in blast - skill expression)

**Voxel Interaction:**
- **Destruction Radius:** 4 units (sphere)
- **Voxel Type Set:** Air (0) - removes voxels
- **Lava Creation:** 2 units radius at center (type 4)
- **Lava Duration:** Permanent (until flows away via physics)

### 5.2 Projectile Behavior

**Launch:**
- Spawns at player camera position
- Travels in straight line (no gravity in MVP)
- Direction = camera forward vector
- Rotation = face direction of travel

**Travel:**
- Constant velocity (no acceleration)
- No homing / tracking
- Passes through destroyed voxels
- Collides with solid voxels → explode
- Collides with enemy → explode
- Collides with nothing for 5 sec → despawn

**Impact:**
1. Trigger explosion effect (particles, sound, shake)
2. Damage all enemies in radius (check Area3D overlaps)
3. Modify voxels (destroy in radius, create lava)
4. Self-destruct projectile

**Edge Cases:**
- Hit multiple enemies? All take damage (splash)
- Hit player? Player takes damage (friendly fire)
- Hit while in air? Explode in air, damage all below
- Hit water? Normal explosion (no special interaction MVP)

### 5.3 Visual Design

**Projectile Appearance:**
- Sphere mesh (0.3 unit radius)
- Emissive orange/red material (glows)
- Trailing particle system:
  - Fire particles
  - Fade out over 0.5 sec
  - Small smoke trail
- Rotation for visual interest

**Explosion Effect:**
- Sphere of fire particles (expands rapidly)
- Flash of light (additive blend)
- Orange/red color
- Screen shake (3 pixels, 0.2 sec)
- Voxel debris particles (small rocks flying out)

**Lava Visual:**
- Existing voxel lava (type 4) already glows
- No additional effect needed
- Lava physics (flows, spreads) already works

### 5.4 Audio Design

**Cast Sound:**
- "FWOOSH" - ignition sound
- 0.3 sec duration
- Punchy start (attack)
- Medium-high pitch
- Fire/magic quality

**Travel Sound:**
- Subtle whoosh (Doppler effect?)
- Quiet, not annoying
- Can be omitted if annoying

**Impact Sound:**
- "BOOM" - explosion
- 0.5 sec duration
- Deep, satisfying thud
- Reverb tail
- Layered:
  - Bass thump (impact)
  - Fire crackle (magic)
  - Rock breaking (voxel destruction)

**Miss Sound (optional):**
- Fizzle if hits nothing
- Communicate failure

### 5.5 Game Feel Checklist

When casting fireball, player should feel:
- [ ] **Power** - This is a dangerous spell
- [ ] **Weight** - Projectile has substance
- [ ] **Impact** - Explosion is LOUD and BIG
- [ ] **Satisfaction** - Seeing the destruction is rewarding
- [ ] **Skill** - Aiming and timing matter
- [ ] **Risk** - Can hurt self (respect the spell)

If any checkbox is unchecked after implementation, iterate until checked.

---

## 6. Enemy Design - The Slime

### 6.1 Why A Slime?

**Advantages:**
- Simple shape (sphere/blob) - easy to model/animate
- Fits theme (magic world, classic RPG enemy)
- Visually distinct from player
- Can be cute or menacing
- Low expectations (it's a slime, not a dragon)

**Behavior Fits MVP:**
- Simple AI (move toward player, attack)
- No complex animations needed
- Clear threat (approaches player)
- Satisfying to kill (squishy, explodes)

### 6.2 Stats & Properties

**Health:**
- HP: 50 (one fireball kill)
- No armor/resistance

**Movement:**
- Speed: 4 units/sec (slower than player sprint)
- Movement type: Slide along ground (no jumping)
- Pathfinding: Direct line to player (no NavigationAgent in MVP)
- Collision: CharacterBody3D, small capsule

**Attack:**
- Type: Melee
- Range: 1.5 units (must touch player)
- Damage: 20 per hit
- Cooldown: 1.5 seconds (can't spam)
- Windup: 0.3 sec (telegraphed)

**Detection:**
- Range: 30 units (always sees player within this range)
- Line of sight: Not needed in MVP (always knows player location)
- Aggro: Instant (no idle state)

### 6.3 AI Behavior (Simple State Machine)

**State: CHASE (Default)**
- Always move directly toward player
- Use velocity to slide across ground
- Maintain forward direction
- If within attack range → transition to ATTACK

**State: ATTACK**
- Stop movement
- Play attack animation (lunge forward)
- Deal damage after windup
- Cooldown timer
- Return to CHASE

**State: DEAD**
- Health = 0
- Play death effect
- Stop all behavior
- Fade out over 1 second
- Remove from scene

**No other states needed** (no idle, patrol, flee, etc.)

### 6.4 Visual Design

**Appearance:**
- Shape: Sphere or rounded blob
- Size: 0.8 units tall, 0.8 units wide (smaller than player)
- Color: Bright green (high visibility against environment)
- Material: Slightly transparent, gelatinous look
- Eyes: Two simple dots (gives personality)

**Animation:**
- Idle/Move: Subtle squash-and-stretch bob (0.2 sec cycle)
- Attack: Quick forward lunge, then snap back
- Hit: Flash white for 0.1 sec (damage feedback)
- Death: Rapid squash, particle burst, fade alpha to 0

**Particles (Death):**
- Green slime particles
- Burst outward in sphere
- Gravity affects (fall down)
- Fade over 1 sec

### 6.5 Audio

**Movement:**
- Squelch sound (subtle, low volume)
- Plays on footsteps / movement

**Attack:**
- Slurp or splat sound
- Indicates attack is happening

**Hurt:**
- Squishy impact sound
- Higher pitch = damage received

**Death:**
- Wet pop/splat
- Satisfying, not gory

### 6.6 Game Feel Checklist

When fighting slime, player should feel:
- [ ] **Threat** - I need to avoid getting hit
- [ ] **Clarity** - I can see the slime clearly
- [ ] **Feedback** - I know when I hit it
- [ ] **Satisfaction** - Killing it feels rewarding
- [ ] **Fairness** - Deaths feel like my fault, not BS

---

## 7. Game Feel & Polish

### 7.1 The Juice List

**Juice** = all the little details that make a game feel great

**Critical Juice (Must Have):**

1. **Screen Shake**
   - On fireball explosion (3 pixels, 0.2 sec)
   - On player taking damage (2 pixels, 0.1 sec)
   - Subtle, not nauseating

2. **Hit Flash**
   - Enemy flashes white when hit (0.1 sec)
   - Player screen red tint when hit (0.2 sec fade)

3. **Damage Numbers**
   - "50" floats up from enemy on hit
   - Yellow/orange color
   - Fades and moves up over 1 sec

4. **Impact Particles**
   - Fireball explosion (fire sphere)
   - Voxel debris (rocks flying)
   - Slime death (green particles)

5. **Sound Effects**
   - Fireball cast, impact, explosion
   - Slime attack, hurt, death
   - Player hurt
   - UI sounds (click, score increment)

6. **Visual Feedback**
   - Crosshair changes color when over enemy (red)
   - Mana bar flashes when low (<20%)
   - Health bar flashes when low (<30%)

**Nice-to-Have Juice:**

7. **Camera Effects**
   - Slight camera tilt when moving (subtle)
   - FOV increase when sprinting (speed feeling)

8. **Advanced Particles**
   - Heat distortion above lava
   - Smoke rising from destroyed voxels
   - Dust clouds from slime movement

9. **Additional Audio**
   - Ambient environment sounds
   - Music (subtle, not distracting)
   - Layered impact sounds (more complex)

10. **Extra Visual Polish**
    - Slime leaves slime trail behind (decals)
    - Explosion light (dynamic light briefly)
    - Glowing eyes on slime (more personality)

### 7.2 Animation Principles

**Even without complex animations, follow these:**

1. **Anticipation** - Slime winds up before attack (0.3 sec)
2. **Squash & Stretch** - Slime squashes when landing, stretches when jumping/attacking
3. **Follow Through** - Slime continues moving slightly after stop (momentum)
4. **Timing** - Fast actions (attack) vs slow actions (idle bob)
5. **Secondary Action** - Eyes blink independently of body movement

**Implementation:**
- Use simple shader/material animation (UV scroll, vertex displacement)
- Tween scale for squash/stretch
- Tween position for bob/wobble

### 7.3 Color & Visual Language

**Enemy Color:**
- Green = enemy (universal gaming language)
- Bright green = high visibility

**Player Effects:**
- Orange/Red = fire (fireball, explosion)
- Blue = mana (mana bar, magic effects)
- Red = health / damage (health bar, screen flash)

**Environment:**
- Rock: Gray/brown (neutral)
- Grass: Green (but different shade from enemy)
- Lava: Orange/yellow (danger)
- Sky: Blue (calming backdrop)

**Consistency:**
- Red always means danger/health
- Orange always means fire/damage
- Green always means enemy
- Blue always means mana/magic

---

## 8. Asset Requirements

### 8.1 3D Models Needed

**Player:**
- None (first-person, no visible body in MVP)

**Enemy (Slime):**
- Option A: Sphere primitive (built-in to Godot)
  - Use sphere mesh
  - Apply material with transparency
  - Shader for squash/stretch via vertex displacement
- Option B: Custom blob model
  - Simple 3D model (can make in Blender in 10 min)
  - ~500 polygons
  - Rigged for basic deformation

**Recommendation:** Use sphere primitive, saves time

**Projectile (Fireball):**
- Sphere primitive (0.3 unit radius)
- Emissive material (orange/red, glow)

**Props (Optional):**
- None needed for MVP

### 8.2 Textures Needed

**Enemy:**
- Slime material (can be procedural shader)
  - Base color: Green
  - Transparency: 0.7
  - Emission: Slight green glow
  - Roughness: 0.3 (shiny)

**Fireball:**
- Emissive texture (gradient sphere)
- Or procedural (fresnel glow)

**Particles:**
- Soft round particle (white, will be colored)
- Can use Godot default particle texture

**UI:**
- None (ProgressBar uses colors, no custom textures needed)

**Total Custom Textures:** 0-2 (can use procedural materials)

### 8.3 Particle Systems Needed

**1. Fireball Trail**
- System: CPUParticles3D or GPUParticles3D
- Emission: Continuous while projectile exists
- Shape: Point at projectile position
- Particle: Small fire particle (orange-yellow gradient)
- Lifetime: 0.5 sec
- Velocity: Opposite of travel direction (trail effect)
- Count: 20-30 particles
- Gravity: Slight upward (fire rises)

**2. Fireball Explosion**
- System: CPUParticles3D (one-shot)
- Emission: Burst on impact (50-100 particles)
- Shape: Sphere
- Particle: Fire + smoke
- Lifetime: 1 sec
- Velocity: Radial outward
- Gravity: Downward for smoke
- Color: Orange → red → gray (fade over lifetime)

**3. Voxel Debris**
- System: CPUParticles3D (one-shot)
- Emission: Burst on explosion (20-30 particles)
- Shape: Sphere
- Particle: Small cube (rock debris)
- Lifetime: 1-2 sec
- Velocity: Radial outward + upward
- Gravity: Strong downward
- Collision: Optional (bounce on ground)

**4. Slime Death**
- System: CPUParticles3D (one-shot)
- Emission: Burst on death (30-40 particles)
- Shape: Sphere
- Particle: Soft particle (green)
- Lifetime: 1 sec
- Velocity: Radial outward
- Gravity: Downward
- Color: Bright green → dark green → transparent

**5. Damage Numbers (Optional but nice)**
- System: Label3D or AnimatedSprite3D
- Creates floating text on damage
- Animates upward and fades out
- Simple to implement with Tween

**Total Particle Systems:** 4 critical + 1 optional

### 8.4 Sound Effects Needed

**Fireball:**
1. Cast sound (FWOOSH) - 0.3 sec
2. Impact sound (BOOM) - 0.5 sec

**Slime:**
3. Movement sound (squelch) - looping, low volume
4. Attack sound (slurp/splat) - 0.3 sec
5. Hurt sound (squishy impact) - 0.2 sec
6. Death sound (wet pop) - 0.4 sec

**Player:**
7. Hurt sound (grunt/impact) - 0.3 sec

**UI:**
8. Score increment (ding) - 0.1 sec
9. Game over (negative sound) - 0.5 sec

**Total Sounds:** 9 (all short, simple)

**Sources:**
- Option A: Free asset sites (freesound.org, opengameart.org)
- Option B: Generate with AI tools (elevenlabs, etc.)
- Option C: Record with phone + edit in Audacity
- Option D: Synthesize in SFXR/Bfxr

**Recommendation:** Mix of free assets + generated

### 8.5 Music (Optional)

**MVP:**
- Not required
- Simple ambient sound or silence is fine
- Focus energy on SFX instead

**If Including:**
- Single looping track (2-3 min)
- Low-key, not distracting
- Electronic or orchestral
- Royalty-free from incompetech.com or similar

**Recommendation:** Skip music for MVP, add post-launch

### 8.6 UI Assets

**Already Have:**
- ProgressBar (health/mana) - Godot built-in

**Need:**
- Crosshair sprite (simple + sign or dot) - can be Label "+"
- Game over screen text
- Score counter

**Total:** 0 custom assets (use Godot built-in UI)

### 8.7 Asset Creation Timeline

**Immediate (Can start now):**
- Placeholder fireball material (emissive orange sphere)
- Placeholder slime material (green sphere)

**Week 1 (During implementation):**
- Fireball particles (trail, explosion)
- Slime death particles
- Voxel debris particles

**Week 2 (Polish phase):**
- Sound effects (gather/create all 9)
- Damage numbers (if implementing)
- Final visual polish

**Total Time Investment:** 1-2 days of asset work across 2 weeks

---

## 9. Implementation Roadmap

### 9.1 Current Status

**✅ Completed (Phase 1):**
- Project structure (game/ folder)
- Player movement (flying/walking)
- Health system (HealthComponent)
- Mana system (ManaComponent)
- Spell casting framework (SpellCaster)
- Basic UI (HUD, health bar, mana bar)
- Excavate spell (proof of concept)
- Main game scene

**Estimated Time Spent:** 2-3 days
**Next Phase:** Phase 2 - Fireball Spell

---

### 9.2 Phase 2: Fireball Spell (Critical Path)

**Goal:** Fireball projectile that explodes and destroys voxels

**Week 1: Day 1-2 - Projectile System**

**Tasks:**
1. Create `spell_projectile.gd` base class
   - Area3D with collision detection
   - Velocity-based movement
   - Lifetime timer (auto-destroy)
   - `body_entered` signal handling
   - `explode()` method (stub)

2. Create `fireball_projectile.tscn`
   - Sphere mesh (visual)
   - SphereShape3D (collision)
   - Emissive orange material
   - Attach `fireball_projectile.gd` script

3. Create `fireball_projectile.gd` (inherits spell_projectile)
   - Override `explode()` method
   - Implement voxel destruction
   - Implement lava creation

4. Create `fireball.tres` SpellData resource
   - spell_type = PROJECTILE
   - mana_cost = 20
   - cooldown = 1.0
   - projectile_scene = fireball_projectile.tscn
   - projectile_speed = 15
   - damage = 50
   - voxel_radius = 4
   - voxel_type = 0 (air for destruction)

5. Test basic projectile
   - Spawns from player camera
   - Flies forward
   - Despawns after 5 sec
   - Detects collision with voxels

**Deliverable:** Fireball projectile flies and explodes (no polish yet)
**Time:** 2 days

---

**Week 1: Day 3 - Explosion & Voxel Interaction**

**Tasks:**
1. Implement explosion in `fireball_projectile.gd`
   - Call `voxel_world.edit_world()` to destroy voxels
   - Call `voxel_world.edit_world()` to create lava (separate call)
   - Test with different radii
   - Ensure timing is correct (destroy then create lava)

2. Implement damage to entities
   - Get all bodies in Area3D when explode
   - For each body with HealthComponent, call `take_damage()`
   - Include player (friendly fire)

3. Test edge cases
   - Hit voxel wall → explode
   - Hit ground → explode + destroy ground
   - Hit nothing → despawn after lifetime
   - Hit player → player takes damage

4. Balance pass
   - Adjust destruction radius (feel satisfying?)
   - Adjust lava radius (too much? too little?)
   - Adjust damage (one-shot slime later)

**Deliverable:** Fireball fully functional (destroys voxels, creates lava, damages)
**Time:** 1 day

---

**Week 1: Day 4 - Particles & Visual Polish**

**Tasks:**
1. Create fireball trail particles
   - CPUParticles3D node in fireball_projectile.tscn
   - Configure: emission shape, color, lifetime
   - Attach to projectile (follows it)
   - Test and tweak (looks good in motion?)

2. Create explosion particle effect
   - CPUParticles3D in separate scene (explosion_effect.tscn)
   - Instantiate on impact
   - Fire burst + smoke
   - Auto-delete after particles finish

3. Create voxel debris particles
   - CPUParticles3D with cube mesh
   - Small rocks flying out
   - Gravity pulls down

4. Gather/create placeholder sounds
   - Find free explosion sound
   - Find free cast sound
   - Add AudioStreamPlayer3D to projectile
   - Play sound on cast, impact

5. Visual polish
   - Adjust projectile emission strength
   - Adjust particle colors
   - Test at different times of day (lighting)

**Deliverable:** Fireball looks and sounds great
**Time:** 1 day

---

**Week 1: Day 5 - Game Feel & Feedback**

**Tasks:**
1. Implement screen shake
   - Create `camera_shake.gd` utility script
   - Add to VoxelCamera
   - Call on explosion (3 pixel shake, 0.2 sec)
   - Tween-based smooth shake

2. Implement damage numbers (optional)
   - Create `damage_number.tscn` (Label3D)
   - Spawn on enemy hit
   - Animate upward with Tween
   - Fade alpha over 1 sec
   - Auto-delete

3. Polish UI feedback
   - Crosshair turns red when aiming at enemy (future, need enemy first)
   - Mana bar flashes when low
   - Screen flash red when player hit

4. Playtesting session (self)
   - Cast fireball 50+ times
   - Note what feels good
   - Note what feels bad
   - Tweak until it feels GREAT

**Deliverable:** Fireball feels amazing to cast
**Time:** 1 day

**Phase 2 Total Time:** 5 days

---

### 9.3 Phase 3: Slime Enemy (Critical Path)

**Goal:** One enemy type that chases and attacks player

**Week 2: Day 1-2 - Enemy Foundation**

**Tasks:**
1. Create `base_enemy.gd` script
   - CharacterBody3D base class
   - HealthComponent child (reuse!)
   - State machine (CHASE, ATTACK, DEAD)
   - Detection logic (distance to player)
   - Movement toward player (velocity-based)
   - Attack logic (cooldown timer)

2. Create `slime.tscn`
   - CharacterBody3D root
   - CapsuleShape3D collision (0.8 units)
   - MeshInstance3D with sphere mesh
   - Green emissive material
   - HealthComponent node (max_health = 50)
   - Attach `slime.gd` script (inherits base_enemy)

3. Implement CHASE state
   - Calculate direction to player
   - Apply velocity toward player
   - Call `move_and_slide()`
   - Update every frame

4. Implement ATTACK state
   - Detect when in range (1.5 units)
   - Stop movement
   - Play attack animation (scale tween lunge)
   - After windup, deal damage to player
   - Cooldown timer

5. Test basic enemy
   - Spawns near player
   - Chases player
   - Attacks when close
   - Player health decreases

**Deliverable:** Slime moves and attacks
**Time:** 2 days

---

**Week 2: Day 3 - Enemy Combat Integration**

**Tasks:**
1. Integrate with fireball
   - Ensure slime has HealthComponent
   - Test fireball explosion hits slime
   - Slime health decreases
   - Log damage to console

2. Implement death
   - Listen to HealthComponent.died signal
   - Transition to DEAD state
   - Stop all behavior
   - Play death effect (particles)
   - Fade out alpha over 1 sec
   - queue_free() after fade

3. Implement hit feedback
   - Flash white when hit (modulate color)
   - Play hurt sound
   - Stagger slightly (push back from impact?)

4. Test combat loop
   - Spawn slime
   - Shoot fireball
   - Slime dies
   - Repeat

**Deliverable:** Can kill slime with fireball
**Time:** 1 day

---

**Week 2: Day 4 - Enemy Spawning & Game Loop**

**Tasks:**
1. Create `enemy_spawner.gd` script
   - Autoload or node in main scene
   - Spawn enemy at interval (5 sec initially)
   - Spawn at random position around player (radius 20-30)
   - Don't spawn in player's view (check angle)
   - Track alive enemies, kills

2. Create `game_manager.gd` (Autoload)
   - Track game state (playing, game_over)
   - Track score (kills)
   - Track time survived
   - Handle game over (player dies)
   - Handle victory (50 kills or 5 min)
   - Provide restart functionality

3. Implement win/lose conditions
   - Player dies (health = 0) → game over
   - Player gets 50 kills → victory
   - Show game over / victory screen
   - Display stats (kills, time)
   - "Restart" button → reload scene

4. Test full loop
   - Game starts
   - Enemy spawns
   - Player kills enemy
   - Another spawns
   - Repeat until win or lose

**Deliverable:** Full game loop works
**Time:** 1 day

---

**Week 2: Day 5 - Enemy Polish**

**Tasks:**
1. Create enemy death particles
   - Green slime burst
   - Configure and tune

2. Add enemy sounds
   - Gather/create sounds (squelch, attack, hurt, death)
   - Add AudioStreamPlayer3D to slime
   - Play sounds on events

3. Add enemy animation
   - Simple squash/stretch with Tween
   - Bob animation while moving
   - Lunge animation on attack
   - Scale to 0 on death (before fade)

4. Visual polish
   - Eyes texture or shader (simple dots)
   - Material adjustments (transparency, glow)
   - Test visibility in different environments

5. Balance pass
   - Adjust movement speed (too fast? too slow?)
   - Adjust detection range
   - Adjust attack damage
   - Adjust spawn rate
   - Goal: Challenging but fair

**Deliverable:** Slime looks and behaves great
**Time:** 1 day

**Phase 3 Total Time:** 5 days

---

### 9.4 Phase 4: UI & Game Loop Polish

**Goal:** Complete UI, smooth game flow, professional feel

**Week 3: Day 1 - UI Improvements**

**Tasks:**
1. Add crosshair
   - Simple + or dot in center
   - ColorRect or Label
   - Changes color when aiming at enemy (red)

2. Add score display
   - Label showing "Kills: X"
   - Updates when enemy dies
   - Top-right corner

3. Add timer
   - Label showing "Time: MM:SS"
   - Updates every frame
   - Next to score

4. Improve health/mana bars
   - Add labels showing numbers (100/100)
   - Add icons (heart, water drop)
   - Better visual style (borders, backgrounds)

5. Add low health/mana warnings
   - Flash effect when low
   - Sound effect when drops below threshold
   - Red tint on health bar

**Deliverable:** UI is clear and informative
**Time:** 1 day

---

**Week 3: Day 2 - Game Over / Victory Screens**

**Tasks:**
1. Create `game_over_screen.tscn`
   - Panel with title "Game Over"
   - Stats display (kills, time survived)
   - "Restart" button
   - "Quit" button
   - Initially hidden

2. Create `victory_screen.tscn`
   - Similar to game over but celebratory
   - "Victory!" title
   - Stats
   - Buttons

3. Implement GameManager transitions
   - On player death → show game over screen
   - On victory condition → show victory screen
   - Pause game during screens (pause tree?)
   - Restart button → reload main scene
   - Quit button → quit game

4. Add screen transitions
   - Fade in when showing screens
   - Fade out when restarting

**Deliverable:** Win/lose screens functional
**Time:** 1 day

---

**Week 3: Day 3 - Audio Implementation**

**Tasks:**
1. Gather all sound effects
   - Fireball cast, impact (done in Phase 2)
   - Slime sounds (done in Phase 3)
   - Player hurt sound
   - UI sounds (score, game over)
   - Collect from free asset sites

2. Implement audio manager (optional)
   - Autoload script for playing sounds
   - Handles sound pooling (reuse AudioStreamPlayer)
   - Volume controls

3. Add 3D spatial audio
   - Enemy sounds from enemy position
   - Explosion sounds from explosion position
   - Use AudioStreamPlayer3D

4. Add UI sounds
   - Click sound on buttons
   - Score increment sound (ding)
   - Game over sound (negative tone)

5. Balance audio levels
   - Mix volumes (nothing too loud/quiet)
   - Test with different volumes
   - Ensure all sounds audible but not overwhelming

**Deliverable:** Complete audio experience
**Time:** 1 day

---

**Week 3: Day 4-5 - Final Polish & Testing**

**Tasks:**
1. Final game feel pass
   - Test entire loop 20+ times
   - Note any jarring moments
   - Note any unclear feedback
   - Fix issues

2. Bug fixing
   - Enemy getting stuck?
   - Fireball hitting through walls?
   - Mana not regenerating?
   - UI not updating?
   - Fix all blocking bugs

3. Performance testing
   - Test with 10+ enemies
   - Check FPS (target 60+)
   - Profile if needed (Godot profiler)
   - Optimize if necessary

4. Balance final tuning
   - Spawn rate
   - Enemy speed/damage
   - Fireball cost/cooldown
   - Goal: Skilled player can reach 50 kills in ~5-7 minutes

5. Visual consistency pass
   - All colors consistent
   - All materials look good
   - No placeholder art remaining
   - Lighting tweaks

6. Playtesting (external)
   - Ask friend/family to play
   - Watch them (don't help!)
   - Note confusions
   - Note frustrations
   - Iterate based on feedback

**Deliverable:** Complete, polished MVP
**Time:** 2 days

**Phase 4 Total Time:** 5 days

---

### 9.5 Total Timeline Summary

| Phase | Goal | Duration | Status |
|-------|------|----------|--------|
| Phase 1 | Core systems | 2-3 days | ✅ DONE |
| Phase 2 | Fireball spell | 5 days | ⏳ TODO |
| Phase 3 | Slime enemy | 5 days | ⏳ TODO |
| Phase 4 | Polish & UI | 5 days | ⏳ TODO |
| **TOTAL** | **MVP Complete** | **~17 days** | **~30% Done** |

**Realistic Timeline:** 3-4 weeks (accounting for iterations, unexpected issues)

---

## 10. Testing & Iteration

### 10.1 Testing Strategy

**Daily Testing (During Development):**
- After each feature, test immediately
- Cast fireball 10+ times (does it work?)
- Kill enemy 5+ times (does it feel good?)
- Play full loop (spawn → kill → spawn)

**Weekly Testing (End of Phase):**
- Full playthrough (5+ min session)
- Note all issues (bug list)
- Note all feel problems (juice list)
- Prioritize fixes for next week

**External Testing (Before Completion):**
- 3-5 playtesters (friends/family)
- No tutorial, see what's confusing
- Record sessions if possible
- Ask questions after
- Iterate on feedback

### 10.2 Key Questions to Answer

**Phase 2 (Fireball):**
- Does casting feel responsive?
- Is the explosion satisfying?
- Is the destruction radius appropriate?
- Does the projectile speed feel good?
- Can I hit moving targets?

**Phase 3 (Enemy):**
- Is the enemy threatening?
- Can I tell when it's about to attack?
- Does killing it feel rewarding?
- Is it too easy/hard?
- Is the spawn rate appropriate?

**Phase 4 (Polish):**
- Is the UI clear?
- Do I know what my goal is?
- Does the game feel polished?
- Are there any annoying bugs?
- Would I play this again?

### 10.3 Metrics to Track

**Quantitative:**
- Time to first death (should be 30-90 sec)
- Kills per session (track improvement)
- Mana usage (do players run out?)
- Time to reach 50 kills (balance target)

**Qualitative:**
- "That felt good!" moments
- "That was frustrating" moments
- Confusions (what's unclear?)
- Suggestions (what do they want?)

### 10.4 Iteration Framework

**After Feedback:**
1. List top 3 issues
2. Categorize: Bug vs Design vs Polish
3. Fix bugs immediately
4. Design issues: Discuss, plan fix
5. Polish issues: Add to list, prioritize
6. Test again

**Example Iteration:**
- Feedback: "Fireball feels weak"
- Analysis: Explosion too small? Sound too quiet? Voxels not destroyed enough?
- Test: Try increasing radius, better sound, more particles
- Result: Feels better? Keep. Still weak? Try something else.

---

## 11. Success Criteria

### 11.1 MVP is Complete When...

**Functional Requirements:**
- [ ] Fireball spell works (cast, fly, explode, destroy voxels, damage)
- [ ] Slime enemy works (spawn, chase, attack, die)
- [ ] Game loop works (spawn → kill → repeat)
- [ ] Win condition works (50 kills or 5 min → victory screen)
- [ ] Lose condition works (0 HP → game over screen)
- [ ] Can restart game
- [ ] UI shows health, mana, score, time
- [ ] No critical bugs (crashes, soft locks)

**Quality Requirements:**
- [ ] Fireball feels satisfying to cast (juice, sound, visuals)
- [ ] Slime feels threatening but fair
- [ ] 60 FPS stable on target hardware
- [ ] Audio is present and mixed well
- [ ] UI is clear and readable
- [ ] Game is fun for 5+ minutes

**Polish Requirements:**
- [ ] Screen shake on explosion
- [ ] Particles on all major events (explosion, death)
- [ ] Sounds on all major events
- [ ] Damage feedback on hit (flash, numbers)
- [ ] No placeholder assets (unless intentional art style)

### 11.2 Success Metrics (Playtesting)

**First Playthrough (No Instructions):**
- Player survives 30-90 seconds ✅
- Player kills at least 1 enemy ✅
- Player understands mana system ✅
- Player wants to try again ✅

**Fifth Playthrough:**
- Player survives 2-5 minutes ✅
- Player gets 15-30 kills ✅
- Player shows improvement (better aim, positioning) ✅

**After 30 Minutes Total Playtime:**
- Player reaches 50 kills at least once ✅
- Player says "That was fun!" ✅
- Player asks "Can I play more?" or "What's next?" ✅

### 11.3 When to Ship MVP

**Ship When:**
- All functional requirements met
- Most quality requirements met
- At least 80% of polish requirements met
- 3+ playtesters give positive feedback
- No critical bugs remain

**Don't Ship When:**
- Core loop doesn't feel good (iterate more)
- Critical bugs exist (fix first)
- Fireball or slime broken (must work)
- Playtesters confused or frustrated (design issues)

**MVP is Not:**
- Feature complete (more to add post-MVP)
- Perfect (iteration continues)
- Final game (just foundation)

**MVP is:**
- Playable start to finish
- Fun for target audience
- Demonstrates potential
- Foundation for expansion

---

## 12. Post-MVP Vision

### 12.1 Immediate Next Steps (Post-MVP Phase 1)

**After MVP Success, Add:**

**2-3 More Spells:**
- Lightning Bolt (instant, single-target, chains)
- Ice Shard (projectile, slows enemies)
- Earth Wall (instant, creates rock wall)

**1-2 More Enemies:**
- Golem (tank, slow, high HP)
- Bat (flyer, swoops at player)

**Goal:** Validate spell variety and enemy variety

**Time:** 1-2 weeks

---

### 12.2 Content Expansion (Post-MVP Phase 2)

**Spell Schools:**
- Fire (fireball, meteor, wall of fire)
- Ice (ice shard, freeze, ice wall)
- Earth (excavate, earth wall, earthquake)
- Lightning (bolt, chain, storm)
- Arcane (teleport, shield, magic missile)

**Total:** 15-20 spells across 5 schools

**Enemy Variety:**
- 10+ unique enemy types
- Different behaviors (ranged, flyer, tank, swarm)
- Enemy factions (fight each other)

**Time:** 4-6 weeks

---

### 12.3 Procedural Generation (Post-MVP Phase 3)

**Terrain Generation:**
- Procedural cave systems
- Procedural floating islands
- Procedural dungeons
- Different biomes (lava, ice, crystal)

**Enemy Spawning:**
- Procedural spawn points
- Biome-specific enemies
- Difficulty scaling with depth

**Time:** 3-4 weeks

---

### 12.4 Meta-Progression (Post-MVP Phase 4)

**Unlock System:**
- Start with 3 basic spells
- Unlock more via currency
- Unlock upgrades (health, mana, spell power)

**Roguelike Elements:**
- Permadeath (lose run progress)
- Keep currency (spend on unlocks)
- Increasing difficulty runs

**Time:** 2-3 weeks

---

### 12.5 Long-Term Features (6+ Months Out)

- Biome variety (10+ distinct environments)
- Boss fights (5-10 unique bosses)
- Spell combos (cast A + B = C effect)
- Enemy AI improvements (tactics, coordination)
- Multiplayer co-op
- Level editor (player-created content)

---

## 13. Final Thoughts

### 13.1 Why This MVP Will Succeed

**Focused Scope:**
- One spell, one enemy = deep not wide
- Master the core loop first
- Easy to expand later

**Clear Vision:**
- Know exactly what we're building
- Know what success looks like
- Know when we're done

**Proven Foundation:**
- Phase 1 works (health, mana, UI)
- Architecture is solid (components, resources)
- Voxel system works (destruction tested)

**Realistic Timeline:**
- 3-4 weeks total
- Clear daily tasks
- No crunch required

### 13.2 Key Principles

1. **Feel over features** - One spell that feels amazing > five that feel ok
2. **Iterate quickly** - Test daily, fix immediately
3. **Polish matters** - Juice makes good feel great
4. **Stay focused** - Resist feature creep, stick to MVP
5. **Have fun** - If we're not enjoying making it, players won't enjoy playing it

### 13.3 Next Steps

1. ✅ Phase 1 complete - Foundation solid
2. ⏳ Phase 2 next - Implement fireball spell
3. Begin asset gathering (sounds, particles)
4. Set up daily testing routine
5. Update this document as we learn

---

**End of Document**

*Let's build something great. One fireball at a time.*
