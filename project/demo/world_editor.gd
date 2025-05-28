extends Node3D

@export var world : VoxelWorld

func _input(event: InputEvent) -> void:
	if event.is_action_pressed("left_click"):
		world.edit_world(global_position, -global_transform.basis.z, 10, 100);
