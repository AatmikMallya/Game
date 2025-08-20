extends Node3D

@export var world : VoxelWorld
@export var selected_material : int = 1;

func set_selected_material(value: int) -> void:
	selected_material = value

func _process(_delta: float) -> void:
	if Input.is_action_pressed("left_click"):
		world.edit_world(global_position, -global_transform.basis.z, 3, 1000, selected_material);
	if Input.is_action_pressed("right_click"):
		world.edit_world(global_position, -global_transform.basis.z, 3, 1000, 0);
