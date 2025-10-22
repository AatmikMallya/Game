extends CharacterBody3D

## Player controller with health, mana, and spell casting
## Based on the demo player_controller.gd but with gameplay components

const MIN_MOVE_SPEED = 0.5
const MAX_MOVE_SPEED = 512

@export var camera: Node3D
@export var speed = 5.0
@export var jump_velocity = 4.5
@export var fly_speed: float = 8.0
@export var look_sensitivity: float = 0.1

var input_active: bool = false
@export var is_flying: bool = false

# Component references (will be set up as child nodes in the scene)
@onready var health_component = $HealthComponent
@onready var mana_component = $ManaComponent
@onready var spell_caster = $SpellCaster

func _ready():
	add_to_group("player")
	set_mouse(true)
	# Ensure gravity/walking by default (not flying)
	is_flying = false
	# Improve ground adherence to avoid tunneling on fast falls
	floor_snap_length = 0.6

	# Snap to voxel terrain top at current XZ
	await get_tree().process_frame
	_snap_to_voxel_ground()

	# Listen to own death
	if health_component:
		health_component.died.connect(_on_player_died)

	# Listen to spell casting feedback
	if spell_caster:
		spell_caster.spell_cast_failed.connect(_on_spell_cast_failed)

func set_mouse(value: bool) -> void:
	input_active = value
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED if input_active else Input.MOUSE_MODE_VISIBLE

func _input(event) -> void:
	# Toggle flying/walking mode
	if event.is_action_pressed("change_player_mode"):
		is_flying = !is_flying

	# Speed adjustment
	if event.is_action_pressed("scroll_up"):
		fly_speed *= 2
	if event.is_action_pressed("scroll_down"):
		fly_speed *= 0.5
	fly_speed = clamp(fly_speed, MIN_MOVE_SPEED, MAX_MOVE_SPEED)

	# Mouse look
	if event is InputEventMouseMotion and input_active:
		var cam: Node3D = camera if camera != null else get_node_or_null("VoxelCamera")
		if cam != null:
			rotation.y += -event.relative.x * 0.025 * look_sensitivity
			cam.rotation.x += -event.relative.y * 0.025 * look_sensitivity
			cam.rotation.x = clamp(cam.rotation.x, -PI/2.2, PI/2.2)

	# Spell casting
	if event.is_action_pressed("left_click"):
		if spell_caster:
			spell_caster.cast_current_spell()

	# Spell selection (1-5 keys)
	if event is InputEventKey and event.pressed:
		if event.keycode >= KEY_1 and event.keycode <= KEY_5:
			var spell_index = event.keycode - KEY_1
			if spell_caster:
				spell_caster.select_spell(spell_index)

	# Debug controls
	if OS.is_debug_build():
		if event.is_action_pressed("ui_page_up"):
			_debug_heal()
		if event.is_action_pressed("ui_page_down"):
			_debug_damage()

func _process_flying(_delta: float) -> void:
	var wish_dir_raw = Input.get_vector("move_left", "move_right", "move_forward", "move_backward")
	var cam: Node3D = camera if camera != null else get_node_or_null("VoxelCamera")
	var direction := Vector3.ZERO
	if cam != null:
		direction = (cam.global_basis * Vector3(wish_dir_raw.x, 0, wish_dir_raw.y)).normalized()
	if Input.is_action_pressed("move_up"):
		direction += Vector3.UP
	if Input.is_action_pressed("move_down"):
		direction += Vector3.DOWN
	velocity = direction.normalized() * fly_speed

func _process_walking(delta: float) -> void:
	if not is_on_floor():
		velocity += get_gravity() * delta

	if Input.is_action_just_pressed("ui_accept") and is_on_floor():
		velocity.y = jump_velocity

	var input_dir := Input.get_vector("move_left", "move_right", "move_forward", "move_backward")
	var direction := (global_basis * Vector3(input_dir.x, 0, input_dir.y)).normalized()
	if direction:
		velocity.x = direction.x * speed
		velocity.z = direction.z * speed
	else:
		velocity.x = move_toward(velocity.x, 0, speed)
		velocity.z = move_toward(velocity.z, 0, speed)

func _physics_process(delta: float) -> void:
	if is_flying:
		_process_flying(delta)
	else:
		_process_walking(delta)

	# Extra safety: voxel-ground catch to prevent tunneling at high fall speeds
	if not is_flying and not is_on_floor() and velocity.y < 0.0:
		var vw: VoxelWorld = get_tree().get_first_node_in_group("voxel_world") as VoxelWorld
		if vw != null:
			var max_fall_step: float = absf(velocity.y) * delta + 0.5
			var origin: Vector3 = global_position
			var hit: Vector4 = vw.raycast_voxels(origin, Vector3.DOWN, 0.0, max(1.0, max_fall_step))
			if hit.w >= 0.0:
				var scale: float = vw.get_scale()
				var world_hit: Vector3 = Vector3(hit.x, hit.y, hit.z) * scale
				# Only snap if below our feet within step range
				if origin.y >= world_hit.y and (origin.y - world_hit.y) <= max_fall_step:
					global_position.y = world_hit.y + 0.6
					velocity.y = 0.0

	move_and_slide()

func _on_player_died():
	print("Player died! Game over.")
	# TODO: Show game over screen, respawn, etc.
	# For now, just respawn
	_respawn()

func _on_spell_cast_failed(reason: String):
	# Could show UI feedback here
	print("Spell cast failed: ", reason)

func _respawn():
	if health_component:
		health_component.reset_health()
	if mana_component:
		mana_component.reset_mana()

	# Reset position (find spawn point or use current position)
	# For now just reset health

func _snap_to_voxel_ground() -> void:
	var vw: VoxelWorld = get_tree().get_first_node_in_group("voxel_world") as VoxelWorld
	if vw == null:
		return
	# Cast from high above straight down to find first solid voxel
	var pos: Vector3 = global_position
	var start_y: float = 10000.0
	var origin: Vector3 = Vector3(pos.x, start_y, pos.z)
	var dir: Vector3 = Vector3.DOWN
	var far_dist: float = start_y + 100.0
	var hit: Vector4 = vw.raycast_voxels(origin, dir, 0.0, far_dist)
	if hit.w >= 0.0:
		var scale: float = vw.get_scale()
		var world_hit: Vector3 = Vector3(hit.x, hit.y, hit.z) * scale
		# Place slightly above to avoid clipping
		global_position = Vector3(pos.x, world_hit.y + 0.6, pos.z)

func _debug_heal():
	if health_component:
		health_component.heal(25)
		print("DEBUG: Healed 25 HP")

func _debug_damage():
	if health_component:
		health_component.take_damage(10)
		print("DEBUG: Took 10 damage")
