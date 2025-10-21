extends Node3D

## Automated Benchmark Controller
## Runs performance tests and outputs detailed statistics

@export var warmup_duration: float = 3.0  # Seconds to stabilize before measuring
@export var test_duration: float = 10.0  # Seconds to run benchmark
@export var output_directory: String = "user://benchmark_results/"
@export var enable_console_output: bool = true
@export var enable_json_output: bool = true
@export var enable_csv_output: bool = true  # Detailed per-frame CSV
@export var enable_detailed_profiling: bool = true  # Track detailed per-frame metrics
@export var camera_rotation_speed: float = 15.0  # Degrees per second (yaw)

# Internal state
var _state = BenchmarkState.WARMUP
var _elapsed_time: float = 0.0
var _warmup_elapsed: float = 0.0
var _test_elapsed: float = 0.0
var _frame_times: Array[float] = []
var _fps_samples: Array[float] = []

# Detailed profiling data
var _frame_data: Array[Dictionary] = []  # Per-frame detailed breakdown
var _frame_number: int = 0

enum BenchmarkState {
	WARMUP,
	TESTING,
	COMPLETE
}

# Configuration data
var _config_data: Dictionary = {}
var _voxel_world: VoxelWorld = null
var _camera: Node3D = null

func _ready() -> void:
	# Capture configuration
	_capture_configuration()

	# Find VoxelWorld and Camera in scene
	_voxel_world = get_node_or_null("../VoxelWorld")
	_camera = get_node_or_null("../Player/VoxelCamera")

	print("=== Voxel Engine Benchmark ===")
	print("Warmup: %.1fs | Test: %.1fs" % [warmup_duration, test_duration])
	print("Starting warmup...")

func _process(delta: float) -> void:
	_elapsed_time += delta

	match _state:
		BenchmarkState.WARMUP:
			_process_warmup(delta)
		BenchmarkState.TESTING:
			_process_testing(delta)
		BenchmarkState.COMPLETE:
			pass  # Do nothing, waiting for auto-quit

func _process_warmup(delta: float) -> void:
	_warmup_elapsed += delta

	if _warmup_elapsed >= warmup_duration:
		print("\n=== Starting Benchmark ===")
		_state = BenchmarkState.TESTING
		_frame_times.clear()
		_fps_samples.clear()

func _process_testing(delta: float) -> void:
	var frame_start_time = Time.get_ticks_usec()
	_test_elapsed += delta

	# Rotate camera for dynamic view (pitch down and up)
	# if _camera:
	# 	var angle = -45.0 + sin(_test_elapsed * 0.5) * 30.0  # Oscillate between -75° and -15°
	# 	_camera.rotation_degrees.x = angle

	# Record basic frame data
	var frame_time_ms = delta * 1000.0
	var fps = 1.0 / delta if delta > 0 else 0.0

	_frame_times.append(frame_time_ms)
	_fps_samples.append(fps)

	# Detailed profiling if enabled
	if enable_detailed_profiling:
		_capture_frame_profile(delta, frame_time_ms, fps, frame_start_time)

	_frame_number += 1

	# Check if test complete
	if _test_elapsed >= test_duration:
		_complete_benchmark()

func _complete_benchmark() -> void:
	_state = BenchmarkState.COMPLETE

	print("\n=== Benchmark Complete ===")
	print("Captured %d frames" % _fps_samples.size())

	# Compute statistics
	var results = _compute_statistics()
	print("Statistics computed")

	# Output results
	if enable_console_output:
		print("Outputting console summary...")
		_output_console_summary(results)

	if enable_json_output:
		print("Saving JSON results...")
		_output_json_results(results)

	if enable_csv_output and enable_detailed_profiling:
		print("Saving detailed CSV data...")
		_output_csv_results()

	# Auto-quit after short delay
	print("\nWaiting before quit...")
	await get_tree().create_timer(2.0).timeout
	print("Quitting...")
	get_tree().quit()

func _capture_configuration() -> void:
	"""Capture system and scene configuration"""
	_config_data = {
		"timestamp": Time.get_datetime_string_from_system(),
		"engine_version": Engine.get_version_info(),
		"resolution": DisplayServer.window_get_size(),
		"vsync_mode": DisplayServer.window_get_vsync_mode(),
		"warmup_time": warmup_duration,
		"test_duration": test_duration
	}

	# Will add VoxelWorld config after scene loads
	call_deferred("_capture_voxel_world_config")

func _capture_voxel_world_config() -> void:
	"""Capture VoxelWorld-specific configuration"""
	if _voxel_world:
		var brick_size = _voxel_world.get_brick_map_size()
		var voxel_size = brick_size * Vector3i(8, 8, 8)

		_config_data["voxel_world"] = {
			"brick_map_size": "%dx%dx%d" % [brick_size.x, brick_size.y, brick_size.z],
			"voxel_count": "%dx%dx%d" % [voxel_size.x, voxel_size.y, voxel_size.z],
			"voxel_scale": _voxel_world.get_scale(),
			"simulation_enabled": _voxel_world.get_simulation_enabled()
		}

func _compute_statistics() -> Dictionary:
	"""Compute comprehensive statistics from collected data"""
	if _fps_samples.is_empty():
		return {}

	# Sort for percentile calculations
	var sorted_fps = _fps_samples.duplicate()
	sorted_fps.sort()

	var sorted_frame_times = _frame_times.duplicate()
	sorted_frame_times.sort()

	# Basic stats
	var avg_fps = _average(_fps_samples)
	var min_fps = sorted_fps[0]
	var max_fps = sorted_fps[-1]

	var avg_frame_time = _average(_frame_times)
	var std_dev_frame_time = _standard_deviation(_frame_times, avg_frame_time)

	# Percentiles
	var fps_percentiles = {
		"p1": _percentile(sorted_fps, 0.01),
		"p5": _percentile(sorted_fps, 0.05),
		"p50": _percentile(sorted_fps, 0.50),
		"p95": _percentile(sorted_fps, 0.95),
		"p99": _percentile(sorted_fps, 0.99)
	}

	var frame_time_percentiles = {
		"p1": _percentile(sorted_frame_times, 0.01),
		"p5": _percentile(sorted_frame_times, 0.05),
		"p50": _percentile(sorted_frame_times, 0.50),
		"p95": _percentile(sorted_frame_times, 0.95),
		"p99": _percentile(sorted_frame_times, 0.99)
	}

	return {
		"config": _config_data,
		"results": {
			"total_frames": _fps_samples.size(),
			"test_duration": _test_elapsed,
			"fps": {
				"average": avg_fps,
				"minimum": min_fps,
				"maximum": max_fps,
				"percentiles": fps_percentiles
			},
			"frame_time_ms": {
				"average": avg_frame_time,
				"std_dev": std_dev_frame_time,
				"percentiles": frame_time_percentiles
			}
		}
	}

func _output_console_summary(results: Dictionary) -> void:
	"""Print human-readable summary to console"""
	var r = results["results"]

	print("\n=== Performance Summary ===")
	print("Frames Captured: %d" % r["total_frames"])
	print("Test Duration: %.2fs" % r["test_duration"])
	print("")
	print("FPS:")
	print("  Average: %.1f" % r["fps"]["average"])
	print("  Minimum: %.1f" % r["fps"]["minimum"])
	print("  Maximum: %.1f" % r["fps"]["maximum"])
	print("  1%%ile:   %.1f" % r["fps"]["percentiles"]["p1"])
	print("  5%%ile:   %.1f" % r["fps"]["percentiles"]["p5"])
	print("  50%%ile:  %.1f" % r["fps"]["percentiles"]["p50"])
	print("  95%%ile:  %.1f" % r["fps"]["percentiles"]["p95"])
	print("  99%%ile:  %.1f" % r["fps"]["percentiles"]["p99"])
	print("")
	print("Frame Time (ms):")
	print("  Average: %.2f" % r["frame_time_ms"]["average"])
	print("  Std Dev: %.2f" % r["frame_time_ms"]["std_dev"])
	print("  1%%ile:   %.2f" % r["frame_time_ms"]["percentiles"]["p1"])
	print("  99%%ile:  %.2f" % r["frame_time_ms"]["percentiles"]["p99"])
	print("")

	# Print configuration if available
	if "voxel_world" in results["config"]:
		var vw = results["config"]["voxel_world"]
		print("Voxel World:")
		print("  Brick Map: %s" % vw["brick_map_size"])
		print("  Voxels: %s" % vw["voxel_count"])
		print("  Scale: %.3f" % vw["voxel_scale"])
		print("  Simulation: %s" % ("Enabled" if vw["simulation_enabled"] else "Disabled"))

func _output_json_results(results: Dictionary) -> void:
	"""Save detailed results as JSON file"""
	# Ensure output directory exists
	DirAccess.make_dir_recursive_absolute(output_directory)

	# Generate filename with timestamp
	var timestamp = Time.get_datetime_string_from_system().replace(":", "-").replace("T", "_")
	var filename = output_directory + "benchmark_%s.json" % timestamp

	# Convert to JSON
	var json_string = JSON.stringify(results, "  ")

	# Write to file
	var file = FileAccess.open(filename, FileAccess.WRITE)
	if file:
		file.store_string(json_string)
		file.close()
		print("Results saved to: %s" % filename)
	else:
		printerr("Failed to save results to: %s" % filename)

# Statistical helper functions
func _average(values: Array) -> float:
	if values.is_empty():
		return 0.0
	var sum = 0.0
	for v in values:
		sum += v
	return sum / values.size()

func _standard_deviation(values: Array, mean: float) -> float:
	if values.is_empty():
		return 0.0
	var variance = 0.0
	for v in values:
		var diff = v - mean
		variance += diff * diff
	return sqrt(variance / values.size())

func _percentile(sorted_values: Array, p: float) -> float:
	"""Calculate percentile from pre-sorted array"""
	if sorted_values.is_empty():
		return 0.0
	var index = int(p * (sorted_values.size() - 1))
	return sorted_values[index]

func _capture_frame_profile(delta: float, frame_time_ms: float, fps: float, frame_start_time: int) -> void:
	"""Capture detailed profiling data for this frame"""
	var profile = {
		"frame": _frame_number,
		"timestamp": _test_elapsed,
		"fps": fps,
		"frame_time_ms": frame_time_ms,
		"delta": delta,

		# Memory stats (bytes)
		"memory_static": Performance.get_monitor(Performance.MEMORY_STATIC),
		"memory_static_max": Performance.get_monitor(Performance.MEMORY_STATIC_MAX),

		# Time monitors (seconds)
		"time_process": Performance.get_monitor(Performance.TIME_PROCESS),
		"time_physics_process": Performance.get_monitor(Performance.TIME_PHYSICS_PROCESS),
		"time_navigation_process": Performance.get_monitor(Performance.TIME_NAVIGATION_PROCESS),

		# Rendering stats
		"render_total_objects": Performance.get_monitor(Performance.RENDER_TOTAL_OBJECTS_IN_FRAME),
		"render_total_primitives": Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME),
		"render_total_draw_calls": Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),
		"render_video_mem_used": Performance.get_monitor(Performance.RENDER_VIDEO_MEM_USED),
		"render_texture_mem_used": Performance.get_monitor(Performance.RENDER_TEXTURE_MEM_USED),
		"render_buffer_mem_used": Performance.get_monitor(Performance.RENDER_BUFFER_MEM_USED),

		# Physics
		"physics_2d_active_objects": Performance.get_monitor(Performance.PHYSICS_2D_ACTIVE_OBJECTS),
		"physics_3d_active_objects": Performance.get_monitor(Performance.PHYSICS_3D_ACTIVE_OBJECTS),
	}

	# Add VoxelWorld detailed timings (ms)
	if _voxel_world:
		profile["voxel_world_total_update"] = _voxel_world.get_time_total_update()
		profile["voxel_world_sim_liquid"] = _voxel_world.get_time_simulation_liquid()
		profile["voxel_world_sim_freeze"] = _voxel_world.get_time_simulation_freeze()
		profile["voxel_world_sim_cleanup"] = _voxel_world.get_time_simulation_cleanup()
		profile["voxel_world_collision"] = _voxel_world.get_time_collision()

		# GPU timings for simulation passes
		profile["gpu_voxel_world_sim_liquid"] = _voxel_world.get_gpu_time_simulation_liquid()
		profile["gpu_voxel_world_sim_freeze"] = _voxel_world.get_gpu_time_simulation_freeze()
		profile["gpu_voxel_world_sim_cleanup"] = _voxel_world.get_gpu_time_simulation_cleanup()

	# Add VoxelCamera detailed timings (ms)
	if _camera:
		profile["voxel_camera_total_render"] = _camera.get_time_total_render()
		profile["voxel_camera_update"] = _camera.get_time_camera_update()
		profile["voxel_camera_raymarching"] = _camera.get_time_raymarching()

		# GPU timing for raymarching
		profile["gpu_voxel_camera_raymarching"] = _camera.get_gpu_time_raymarching()

	_frame_data.append(profile)

func _output_csv_results() -> void:
	"""Output detailed per-frame data to CSV"""
	if _frame_data.is_empty():
		return

	# Ensure output directory exists
	DirAccess.make_dir_recursive_absolute(output_directory)

	# Generate filename with timestamp
	var timestamp = Time.get_datetime_string_from_system().replace(":", "-").replace("T", "_")
	var filename = output_directory + "benchmark_%s_detailed.csv" % timestamp

	# Build CSV content
	var csv_lines: Array[String] = []

	# Header
	var headers = _frame_data[0].keys()
	csv_lines.append(",".join(headers))

	# Data rows
	for frame in _frame_data:
		var values: Array[String] = []
		for key in headers:
			values.append(str(frame[key]))
		csv_lines.append(",".join(values))

	# Write to file
	var file = FileAccess.open(filename, FileAccess.WRITE)
	if file:
		file.store_string("\n".join(csv_lines))
		file.close()
		print("Detailed CSV saved to: %s" % filename)
		print("  Total frames captured: %d" % _frame_data.size())
	else:
		printerr("Failed to save CSV to: %s" % filename)
