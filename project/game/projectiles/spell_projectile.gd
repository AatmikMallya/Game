extends Area3D
class_name SpellProjectile

## Base class for spell projectiles. Handles movement, lifetime, and collision via raycasts.

@export var speed: float = 15.0
@export var lifetime: float = 5.0
@export var radius_visual: float = 0.3
@export var explosion_radius: float = 4.0
@export var lava_radius: float = 2.0
@export var damage: int = 50

var _velocity: Vector3 = Vector3.ZERO
var _age: float = 0.0
var _active: bool = false
var _voxel_world: VoxelWorld = null
var _camera: Node3D = null

# Ray-marched sphere registration (via VoxelCamera)
var _voxel_camera: Node = null
var _vm_id: int = -1

func setup(spell: SpellData, direction: Vector3, voxel_world: VoxelWorld, camera: Node3D = null) -> void:
	# Configure from spell
	if spell:
		speed = spell.projectile_speed
		lifetime = spell.projectile_lifetime
		damage = spell.damage

	_velocity = direction.normalized() * speed
	_voxel_world = voxel_world
	_camera = camera
	_active = true

	# Register with VoxelCamera for ray-marched rendering
	# Try to find a VoxelCamera near by (player's camera)
	if _camera and _camera.has_method("register_projectile"):
		_voxel_camera = _camera
	elif owner and owner.has_node("VoxelCamera"):
		_voxel_camera = owner.get_node("VoxelCamera")
	else:
		# Search in scene (fallback)
		_voxel_camera = get_tree().get_first_node_in_group("voxel_camera")

	if _voxel_camera and _voxel_camera.has_method("register_projectile"):
		_vm_id = _voxel_camera.register_projectile(global_position, radius_visual)

func _ready():
	monitoring = false  # We use raycasts for reliable collision
	set_process(true)
	set_physics_process(true)

func _exit_tree():
	_unregister_vm()

func _physics_process(delta: float) -> void:
	if not _active:
		return

	_age += delta
	if _age >= lifetime:
		queue_free()
		return

	var from_pos: Vector3 = global_position
	var to_pos: Vector3 = from_pos + _velocity * delta

	# 1) Raycast against voxel world via compute (authoritative terrain collision)
	if _voxel_world:
		var dir := _velocity.normalized() if _velocity != Vector3.ZERO else Vector3.ZERO
		var max_range := _velocity.length() * delta + 0.001
		if dir != Vector3.ZERO and max_range > 0.0:
			var hit_v4: Vector4 = _voxel_world.raycast_voxels(from_pos, dir, 0.0, max_range)
			if hit_v4.w >= 0.0:
				# raycast returns voxel grid coordinates; convert to world units
				var scale := _voxel_world.get_scale()
				global_position = Vector3(hit_v4.x, hit_v4.y, hit_v4.z) * scale
				_explode()
				return

	# 2) Fallback: physics raycast for bodies (enemies/props)
	var space: PhysicsDirectSpaceState3D = get_world_3d().direct_space_state
	var query := PhysicsRayQueryParameters3D.create(from_pos, to_pos)
	query.collide_with_areas = false
	query.collide_with_bodies = true
	var result: Dictionary = space.intersect_ray(query)
	if result and result.size() > 0:
		global_position = result["position"]
		_explode()
		return

	# Move forward
	global_position = to_pos

	# Update ray-marched sphere position
	if _voxel_camera and _vm_id >= 0:
		_voxel_camera.update_projectile(_vm_id, global_position, radius_visual)

func _explode():
	# Modify voxels: destroy then create lava
	if _voxel_world:
		_voxel_world.edit_sphere_at(global_position, explosion_radius, 0) # air
		if lava_radius > 0.0:
			_voxel_world.edit_sphere_at(global_position, lava_radius, 4) # lava

	# TODO: damage entities in radius (Phase 3 when enemies exist)

	# Cleanup
	queue_free()

func _unregister_vm():
	if _voxel_camera and _vm_id >= 0:
		_voxel_camera.remove_projectile(_vm_id)
		_vm_id = -1
		_voxel_camera = null
