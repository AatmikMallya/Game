extends "res://game/projectiles/spell_projectile.gd"

## Fireball projectile: orange emissive sphere with optional particles/sound.

@export var visual_radius: float = 0.3

func _ready():
	# Ensure base behavior
	super._ready()
	# Build simple visual representation so we can debug alongside ray-marched sphere
	# Note: This MeshInstance3D will not be visible in the ray-marched view, but helps with physics debug.
	if not has_node("MeshInstance3D"):
		var mesh_instance := MeshInstance3D.new()
		mesh_instance.name = "MeshInstance3D"
		var sphere := SphereMesh.new()
		sphere.radius = visual_radius
		mesh_instance.mesh = sphere
		var mat := StandardMaterial3D.new()
		mat.emission_enabled = true
		mat.emission = Color(1.2, 0.4, 0.1)
		mat.emission_energy_multiplier = 2.0
		mesh_instance.material_override = mat
		add_child(mesh_instance)

func setup(spell: SpellData, direction: Vector3, voxel_world: VoxelWorld, camera: Node3D = null) -> void:
	# Offset spawn a bit forward to avoid self-collision
	global_position += direction.normalized() * 0.6
	await get_tree().process_frame
	super.setup(spell, direction, voxel_world, camera)
