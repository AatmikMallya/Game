extends CharacterBody3D
class_name SlimeSphere

## Simple enemy rendered as ray-marched spheres via VoxelCamera.
## Uses a single physics sphere for collisions so spell projectiles can raycast against it.

@export var base_radius: float = 0.7
@export var wobble_amount: float = 0.06
@export var wobble_speed: float = 2.0
@export var extra_spheres: Array[Vector3] = [Vector3.ZERO] ## local offsets for additional spheres (optional)
@export var extra_radii: Array[float] = [0.7]               ## radii for extra spheres (match length with extra_spheres)

@onready var health: HealthComponent = null

var _vm_ids: Array[int] = []
var _voxel_camera: Node = null
var _voxel_world: Node = null
var _t: float = 0.0

func _ready() -> void:
    add_to_group("enemy")

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
    _voxel_world = get_tree().get_first_node_in_group("voxel_world")

    # Ensure arrays have at least one sphere
    if extra_spheres.is_empty():
        extra_spheres = [Vector3.ZERO]
    if extra_radii.size() != extra_spheres.size():
        extra_radii.resize(extra_spheres.size())
        for i in extra_radii.size():
            if extra_radii[i] <= 0.0:
                extra_radii[i] = base_radius

    # Defer registration by one frame to allow VoxelCamera to initialize
    _defer_register()

    set_physics_process(true)
    set_process(true)

func _exit_tree() -> void:
    _unregister_vm()

func _defer_register() -> void:
    await get_tree().process_frame
    await get_tree().process_frame
    _register_spheres()

func _register_spheres() -> void:
    if not (_voxel_camera and _voxel_camera.has_method("register_projectile")):
        return
    _vm_ids.clear()
    for i in extra_spheres.size():
        var center: Vector3 = global_position + extra_spheres[i]
        var r: float = max(0.01, extra_radii[i])
        var id: int = _voxel_camera.register_projectile(center, r) as int
        _vm_ids.push_back(id)

func _process(delta: float) -> void:
    _t += delta
    # Wobble radii slightly to give life
    var wobble: float = wobble_amount * sin(_t * wobble_speed)

    if _voxel_camera and _vm_ids.size() == extra_spheres.size():
        for i in extra_spheres.size():
            var center: Vector3 = global_position + extra_spheres[i]
            var r: float = max(0.01, extra_radii[i] + wobble)
            var id: int = _vm_ids[i]
            if id >= 0:
                _voxel_camera.update_projectile(id, center, r)

    _check_projectile_hits()

func _physics_process(_delta: float) -> void:
    # Placeholder idle bobbing; hook up AI later
    var o: Vector3 = global_position
    o.y += 0.0
    global_position = o

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

        # Direct overlap check (projectile sphere vs any slime sphere)
        var hit: bool = false
        for i in extra_spheres.size():
            var center: Vector3 = global_position + extra_spheres[i]
            var r: float = max(0.01, extra_radii[i])
            if center.distance_to(ppos) <= (r + proj.radius_visual):
                hit = true
                break
        if hit:
            _apply_damage(proj.damage)
            proj._explode()
            continue

        # Explosion radius pre-check (if close enough, consider damage on impact)
        for i in extra_spheres.size():
            var center2: Vector3 = global_position + extra_spheres[i]
            var r2: float = max(0.01, extra_radii[i])
            if center2.distance_to(ppos) <= (r2 + proj.explosion_radius):
                # Let projectile handle its explosion on world hit; we pre-apply damage once.
                _apply_damage(proj.damage)
                break

func _apply_damage(amount: int) -> void:
    if health:
        health.take_damage(amount)

func _on_died() -> void:
    _unregister_vm()
    queue_free()

func _unregister_vm() -> void:
    if _voxel_camera:
        for i in _vm_ids.size():
            var id: int = _vm_ids[i]
            if id >= 0:
                _voxel_camera.remove_projectile(id)
    _vm_ids.clear()
