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

func _debug_heal():
	if health_component:
		health_component.heal(25)
		print("DEBUG: Healed 25 HP")

func _debug_damage():
	if health_component:
		health_component.take_damage(10)
		print("DEBUG: Took 10 damage")
