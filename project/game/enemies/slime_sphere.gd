extends CharacterBody3D
class_name SlimeSphere

## Simple enemy rendered as ray-marched spheres via VoxelCamera.
## Uses a single physics sphere for collisions so spell projectiles can raycast against it.

@export var base_radius: float = 0.7
@export var aabb_size: Vector3 = Vector3(1.4, 1.4, 1.4)  ## AABB size for entity rendering
@export var entity_scale: float = 0.1  ## Voxel scale for entity

@onready var health: HealthComponent = null

var _entity_id: int = -1
var _voxel_camera: Node = null
var _voxel_world: VoxelWorld = null

func _ready() -> void:
    add_to_group("enemy")
    # Improve ground adherence to avoid tunneling
    floor_snap_length = max(0.2, base_radius * 0.6)

    # Attach or locate health component
    if health == null:
        health = get_node_or_null("Health") as HealthComponent
    if health == null:
        health = HealthComponent.new()
        add_child(health)
    health.max_health = 100
    health.reset_health()
    health.died.connect(_on_died)

    # Find VoxelCamera and VoxelWorld
    _voxel_camera = get_tree().get_first_node_in_group("voxel_camera")
    _voxel_world = get_tree().get_first_node_in_group("voxel_world") as VoxelWorld

    # Snap to terrain before registering
    await get_tree().process_frame
    _snap_to_voxel_ground()
    # Defer registration by a couple of frames to allow VoxelCamera to initialize
    await get_tree().process_frame
    await get_tree().process_frame
    _register_entity()

    set_physics_process(true)
    set_process(true)

func _exit_tree() -> void:
    _unregister_vm()

func _register_entity() -> void:
    if not (_voxel_camera and _voxel_camera.has_method("register_entity")):
        print("SlimeSphere: No VoxelCamera found or register_entity method missing")
        return
    _entity_id = _voxel_camera.register_entity(global_transform, aabb_size, entity_scale)
    print("SlimeSphere: Registered as entity with ID ", _entity_id)

func _process(delta: float) -> void:
    # Update entity transform
    if _voxel_camera and _entity_id >= 0 and _voxel_camera.has_method("update_entity"):
        _voxel_camera.update_entity(_entity_id, global_transform)

    _check_projectile_hits()

func _physics_process(_delta: float) -> void:
    # Apply gravity and slide on terrain
    if not is_on_floor():
        velocity += get_gravity() * _delta
    else:
        # Dampen residual vertical velocity when grounded
        if absf(velocity.y) < 0.01:
            velocity.y = 0.0

    # Extra safety: voxel-ground catch to prevent tunneling
    if not is_on_floor() and velocity.y < 0.0 and _voxel_world != null:
        var max_step: float = absf(velocity.y) * _delta + 0.5
        var origin: Vector3 = global_position
        var hit: Vector4 = _voxel_world.raycast_voxels(origin, Vector3.DOWN, 0.0, max(1.0, max_step))
        if hit.w >= 0.0:
            var scale: float = _voxel_world.get_scale()
            var world_hit: Vector3 = Vector3(hit.x, hit.y, hit.z) * scale
            if origin.y >= world_hit.y and (origin.y - world_hit.y) <= max_step:
                global_position.y = world_hit.y + base_radius + 0.1
                velocity.y = 0.0

    move_and_slide()

func _snap_to_voxel_ground() -> void:
    if _voxel_world == null:
        _voxel_world = get_tree().get_first_node_in_group("voxel_world") as VoxelWorld
    if _voxel_world == null:
        return
    var pos: Vector3 = global_position
    var start_y: float = 10000.0
    var origin: Vector3 = Vector3(pos.x, start_y, pos.z)
    var dir: Vector3 = Vector3.DOWN
    var far_dist: float = start_y + 100.0
    var hit: Vector4 = _voxel_world.raycast_voxels(origin, dir, 0.0, far_dist)
    if hit.w >= 0.0:
        var scale: float = _voxel_world.get_scale()
        var world_hit: Vector3 = Vector3(hit.x, hit.y, hit.z) * scale
        global_position = Vector3(pos.x, world_hit.y + base_radius + 0.1, pos.z)

func _check_projectile_hits() -> void:
    # Poll nearby projectiles and apply damage on overlap or explosion radius.
    var projectiles: Array = get_tree().get_nodes_in_group("projectile")
    if projectiles.is_empty():
        return

    for p in projectiles:
        if not (p is SpellProjectile):
            continue
        var proj: SpellProjectile = p as SpellProjectile
        var ppos: Vector3 = proj.global_position

        # Simple distance check using base_radius
        var dist: float = global_position.distance_to(ppos)
        if dist <= (base_radius + proj.radius_visual):
            _apply_damage(proj.damage)
            proj._explode()
            continue

        # Explosion radius check
        if dist <= (base_radius + proj.explosion_radius):
            _apply_damage(proj.damage)

func _apply_damage(amount: int) -> void:
    if health:
        health.take_damage(amount)

func _on_died() -> void:
    _unregister_vm()
    queue_free()

func _unregister_vm() -> void:
    if _voxel_camera and _entity_id >= 0 and _voxel_camera.has_method("remove_entity"):
        _voxel_camera.remove_entity(_entity_id)
        _entity_id = -1
