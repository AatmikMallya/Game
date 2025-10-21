extends Node
class_name SpellCaster

## Component that handles spell casting
## Manages mana consumption, cooldowns, and spell execution

signal spell_cast(spell_data: SpellData)
signal spell_cast_failed(reason: String)
signal current_spell_changed(spell_data: SpellData)

@export var spell_slots: Array[SpellData] = []  ## Assign spells in editor
@export var current_spell_index: int = 0

var cooldowns: Dictionary = {}  ## spell_name -> time_remaining

@onready var mana_component: ManaComponent = get_parent().get_node_or_null("ManaComponent")
@onready var voxel_world = get_tree().get_first_node_in_group("voxel_world")

func _ready():
	# Get camera from parent (assumes player has VoxelCamera child)
	pass

func _process(delta: float):
	# Update cooldowns
	for spell_name in cooldowns.keys():
		cooldowns[spell_name] = max(0, cooldowns[spell_name] - delta)

func cast_current_spell() -> void:
	if spell_slots.is_empty() or current_spell_index >= spell_slots.size():
		spell_cast_failed.emit("No spell selected")
		return

	var spell = spell_slots[current_spell_index]
	cast_spell(spell)

func cast_spell(spell: SpellData) -> void:
	if not spell:
		return

	# Check cooldown
	if cooldowns.get(spell.spell_name, 0) > 0:
		spell_cast_failed.emit("On cooldown")
		return

	# Check mana
	if mana_component and not mana_component.has_mana(spell.mana_cost):
		spell_cast_failed.emit("Not enough mana")
		return

	# Consume mana
	if mana_component:
		mana_component.use_mana(spell.mana_cost)

	# Start cooldown
	cooldowns[spell.spell_name] = spell.cooldown

	# Execute spell based on type
	match spell.spell_type:
		SpellData.SpellType.PROJECTILE:
			_cast_projectile(spell)
		SpellData.SpellType.INSTANT:
			_cast_instant(spell)
		SpellData.SpellType.AOE:
			_cast_aoe(spell)
		SpellData.SpellType.CHANNEL:
			_cast_channel(spell)

	spell_cast.emit(spell)

	# TODO: Play cast sound
	# if spell.cast_sound:
	#     AudioManager.play_sound(spell.cast_sound)

func _cast_projectile(spell: SpellData) -> void:
	if not spell.projectile_scene:
		push_error("Projectile spell missing projectile_scene")
		return

	var camera = _get_camera()
	if not camera:
		return

	var projectile = spell.projectile_scene.instantiate()
	get_tree().root.add_child(projectile)

	# Position at camera
	projectile.global_position = camera.global_position

	# Set direction (camera forward)
	var direction = -camera.global_transform.basis.z

	# Initialize projectile (assumes it has setup method)
	if projectile.has_method("setup"):
		projectile.setup(spell, direction, voxel_world, camera)

func _cast_instant(spell: SpellData) -> void:
	var camera = _get_camera()
	if not camera:
		return

	# For voxel manipulation spells
	if spell.modifies_voxels and voxel_world:
		var origin = camera.global_position
		var direction = -camera.global_transform.basis.z

		# Call edit_world on voxel world
		voxel_world.edit_world(
			origin,
			direction,
			spell.voxel_radius,
			spell.max_range,
			spell.voxel_type
		)

	# If spell has custom effect script
	if spell.effect_script:
		var effect = spell.effect_script.new()
		if effect.has_method("cast"):
			var origin = camera.global_position
			var direction = -camera.global_transform.basis.z
			effect.cast(origin, direction, voxel_world, spell)

func _cast_aoe(spell: SpellData) -> void:
	# Similar to instant but affects area
	_cast_instant(spell)

func _cast_channel(spell: SpellData) -> void:
	# TODO: Implement channeling logic
	push_warning("Channel spells not yet implemented")

func select_spell(index: int) -> void:
	if index >= 0 and index < spell_slots.size():
		current_spell_index = index
		current_spell_changed.emit(spell_slots[index])

func get_current_spell() -> SpellData:
	if current_spell_index < spell_slots.size():
		return spell_slots[current_spell_index]
	return null

func is_on_cooldown(spell_name: String) -> bool:
	return cooldowns.get(spell_name, 0) > 0

func get_cooldown_remaining(spell_name: String) -> float:
	return cooldowns.get(spell_name, 0)

func get_cooldown_progress(spell_name: String) -> float:
	## Returns 0.0 (on cooldown) to 1.0 (ready)
	var spell = _find_spell_by_name(spell_name)
	if not spell:
		return 1.0

	var remaining = cooldowns.get(spell_name, 0)
	if spell.cooldown == 0:
		return 1.0

	return 1.0 - (remaining / spell.cooldown)

func _find_spell_by_name(spell_name: String) -> SpellData:
	for spell in spell_slots:
		if spell and spell.spell_name == spell_name:
			return spell
	return null

func _get_camera() -> Node3D:
	## Get camera from parent entity
	var parent = get_parent()
	if not parent:
		return null

	# Try to find VoxelCamera
	var camera = parent.get_node_or_null("VoxelCamera")
	if not camera:
		# Try Camera3D
		camera = parent.get_node_or_null("Camera3D")

	return camera
