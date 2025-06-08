extends Node3D

@export var world : VoxelWorld

func _process(delta: float) -> void:
	if Input.is_action_pressed("left_click"):
		world.edit_world(global_position, -global_transform.basis.z, 10, 1000, 1);
	if Input.is_action_pressed("right_click"):
		world.edit_world(global_position, -global_transform.basis.z, 10, 1000, 3);
	if Input.is_action_pressed("middle_click"):
		world.edit_world(global_position, -global_transform.basis.z, 10, 1000, 2);
