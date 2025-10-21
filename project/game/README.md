# Game Implementation - Phase 1: Core Systems

This directory contains the gameplay implementation for the voxel magic combat game.

## What's Been Created

### ✅ Phase 1 - Core Systems (COMPLETE)

All files have been created programmatically and are ready to test!

#### Components (`player/components/`)
- **health_component.gd** - Reusable health management
- **mana_component.gd** - Mana with auto-regeneration
- **spell_caster.gd** - Spell casting with cooldowns

#### Resources
- **spell_data.gd** - Resource script for defining spells
- **excavate.tres** - First spell (removes voxels)

#### UI (`ui/`)
- **hud.tscn** - Main HUD with health/mana bars
- **health_bar.tscn** - Health display
- **mana_bar.tscn** - Mana display

#### Player
- **player.tscn** - Complete player with all components
- **player.gd** - Enhanced player controller

#### Main Scene
- **main.tscn** - Complete game scene ready to run

## How to Test

### 1. Open the Game Scene
1. Open Godot
2. Navigate to `game/main.tscn`
3. Press F5 or click "Play Scene"

### 2. Test Checklist

**Movement:**
- ✓ WASD - Move
- ✓ Mouse - Look around
- ✓ E/Q - Fly up/down (in flying mode)
- ✓ R - Toggle flying/walking mode
- ✓ Scroll wheel - Adjust fly speed

**Health System:**
- ✓ PageUp - Heal (debug)
- ✓ PageDown - Take damage (debug)
- ✓ Watch health bar update

**Mana System:**
- ✓ Watch mana bar regenerate automatically
- ✓ Mana decreases when casting spells

**Spell Casting:**
- ✓ Left Click - Cast excavate spell
- ✓ Voxels should be removed in front of you
- ✓ Mana should decrease by 10
- ✓ Can't cast without enough mana
- ✓ Cooldown prevents spam (0.5s between casts)

### 3. Expected Behavior

**On Start:**
- Player spawns at position (32, 66, 32)
- Health bar shows 100/100
- Mana bar shows 100/100
- Mana starts regenerating (10/sec)

**When Casting Excavate:**
- Left-click to cast
- Sphere of voxels (radius 3) removed in front of camera
- Mana decreases by 10
- Cooldown for 0.5 seconds
- If mana < 10, spell fails (check console for message)

**Health System:**
- PageDown deals 10 damage
- PageUp heals 25 HP
- At 0 HP, player "dies" and respawns (health/mana reset)

## Architecture Overview

```
Main.tscn
├── VoxelWorld (C++)
│   └── Handles voxel rendering/physics
├── Player (CharacterBody3D)
│   ├── HealthComponent (Node)
│   ├── ManaComponent (Node)
│   └── SpellCaster (Node)
│       └── spell_slots[0] = excavate.tres
└── CanvasLayer
    └── HUD
        ├── HealthBar
        └── ManaBar
```

## How Systems Work Together

1. **Player presses Left Click**
2. **player.gd** receives input, calls `spell_caster.cast_current_spell()`
3. **SpellCaster** checks mana and cooldown
4. If valid, **ManaComponent.use_mana()** is called
5. **SpellCaster** executes spell's `effect_script.cast()`
6. **excavate_spell.gd** calls `voxel_world.edit_world()` to remove voxels
7. **ManaComponent** emits `mana_changed` signal
8. **HUD** receives signal and updates mana bar

## Next Steps (Future Phases)

### Phase 2 - Projectile Spells
- Create projectile base class
- Implement fireball spell
- Add spell hotbar UI

### Phase 3 - Enemy System
- Create base enemy with AI
- Implement pathfinding
- Enemy combat

### Phase 4 - Complete Game Loop
- Enemy spawning system
- Win/lose conditions
- Score tracking

## Troubleshooting

**"No spell selected" error:**
- Check that excavate.tres is assigned to Player → SpellCaster → spell_slots

**Health/Mana bars not visible:**
- Check that HUD → player reference points to Player node

**Voxels not being removed:**
- Check that VoxelWorld is in "voxel_world" group
- Check console for errors

**Mana not regenerating:**
- Check that ManaComponent is a child of Player
- Check mana_regen_rate is set (default 10.0)

## Adding New Spells

1. Create spell effect script in `spells/spell_name/`
2. Create .tres resource in `resources/spells/`
3. Configure spell properties
4. Add to player's SpellCaster spell_slots
5. Test!

Example: To add fireball (Phase 2):
- Create `spells/fireball/fireball_projectile.tscn`
- Create `resources/spells/fireball.tres`
- Set spell_type = PROJECTILE
- Assign projectile_scene to fireball_projectile.tscn
