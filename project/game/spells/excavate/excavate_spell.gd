extends Node

## Excavate spell effect
## This script is attached to a SpellData resource via effect_script
## Removes voxels in a sphere at the target location

func cast(origin: Vector3, direction: Vector3, voxel_world, spell_data: SpellData):
	if not voxel_world:
		push_error("Excavate: No voxel world reference")
		return

	# Call edit_world to remove voxels
	# voxel_type = 0 means air (remove voxels)
	voxel_world.edit_world(
		origin,
		direction,
		spell_data.voxel_radius,
		spell_data.max_range,
		0  # Air = remove voxels
	)

	# TODO: Play excavate sound/particles
	print("Excavated voxels at direction: ", direction)
