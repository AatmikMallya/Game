extends Resource
class_name SpellData

## Resource defining a spell's properties
## Create .tres files with this script to define different spells

enum SpellType {
	PROJECTILE,    ## Shoots a projectile (fireball, ice bolt)
	INSTANT,       ## Instant effect at target location (excavate, heal)
	CHANNEL,       ## Hold to channel effect (beam, drain)
	AOE            ## Area effect at location (explosion, rain)
}

## Display
@export var spell_name: String = "Unnamed Spell"
@export_multiline var description: String = ""
@export var icon: Texture2D  ## For UI display

## Type & Mechanics
@export var spell_type: SpellType = SpellType.INSTANT
@export var mana_cost: float = 10.0
@export var cooldown: float = 1.0
@export var cast_time: float = 0.0  ## 0 for instant cast

## For projectile spells
@export var projectile_scene: PackedScene
@export var projectile_speed: float = 20.0
@export var projectile_lifetime: float = 5.0

## For instant/AOE spells (GDScript that implements cast() method)
@export var effect_script: GDScript

## Damage & Effects
@export var damage: int = 0
@export var knockback_force: float = 0.0

## Voxel Manipulation
@export var modifies_voxels: bool = false
@export var voxel_radius: int = 3
@export var voxel_type: int = 0  ## 0=air, 1=rock, 2=sand, 3=water, 4=lava

## Targeting
@export var max_range: float = 50.0
@export var requires_target: bool = false

## Visual/Audio
@export var cast_sound: AudioStream
@export var impact_sound: AudioStream
@export var cast_particles: PackedScene
@export var impact_particles: PackedScene
