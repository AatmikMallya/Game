extends Control

## Real-time Benchmark Stats Display
## Shows live performance metrics during benchmark run

@export var update_interval: float = 0.1  # Update UI every N seconds
@export var show_detailed_stats: bool = true

var _controller: Node3D = null
var _update_timer: float = 0.0

# UI Labels (will be created programmatically)
var _status_label: Label
var _fps_label: Label
var _frame_time_label: Label
var _progress_bar: ProgressBar
var _details_label: Label

func _ready() -> void:
	# Find the benchmark controller
	_controller = get_node_or_null("../../BenchmarkController")

	if not _controller:
		printerr("BenchmarkPanel: Could not find BenchmarkController")
		return

	_setup_ui()

func _setup_ui() -> void:
	"""Create UI elements programmatically"""
	# Main container
	var vbox = VBoxContainer.new()
	vbox.anchor_right = 1.0
	vbox.anchor_bottom = 1.0
	vbox.offset_left = 20
	vbox.offset_top = 20
	vbox.offset_right = -20
	vbox.offset_bottom = -20
	add_child(vbox)

	# Title
	var title = Label.new()
	title.text = "=== Voxel Engine Benchmark ==="
	title.add_theme_font_size_override("font_size", 24)
	vbox.add_child(title)

	vbox.add_child(_create_spacer(10))

	# Status
	_status_label = Label.new()
	_status_label.add_theme_font_size_override("font_size", 18)
	vbox.add_child(_status_label)

	# Progress bar
	_progress_bar = ProgressBar.new()
	_progress_bar.custom_minimum_size = Vector2(400, 30)
	_progress_bar.max_value = 100
	_progress_bar.value = 0
	vbox.add_child(_progress_bar)

	vbox.add_child(_create_spacer(20))

	# FPS Display
	_fps_label = Label.new()
	_fps_label.add_theme_font_size_override("font_size", 32)
	_fps_label.add_theme_color_override("font_color", Color.GREEN)
	vbox.add_child(_fps_label)

	# Frame Time Display
	_frame_time_label = Label.new()
	_frame_time_label.add_theme_font_size_override("font_size", 18)
	vbox.add_child(_frame_time_label)

	if show_detailed_stats:
		vbox.add_child(_create_spacer(20))

		# Detailed stats
		_details_label = Label.new()
		_details_label.add_theme_font_size_override("font_size", 14)
		_details_label.add_theme_color_override("font_color", Color(0.8, 0.8, 0.8))
		vbox.add_child(_details_label)

func _create_spacer(height: float) -> Control:
	var spacer = Control.new()
	spacer.custom_minimum_size = Vector2(0, height)
	return spacer

func _process(delta: float) -> void:
	if not _controller:
		return

	_update_timer += delta

	# Update UI at specified interval
	if _update_timer >= update_interval:
		_update_timer = 0.0
		_update_display()

func _update_display() -> void:
	"""Update all UI elements with current benchmark state"""
	var state = _controller._state
	var elapsed = _controller._elapsed_time
	var warmup_elapsed = _controller._warmup_elapsed
	var test_elapsed = _controller._test_elapsed
	var warmup_duration = _controller.warmup_duration
	var test_duration = _controller.test_duration

	# Calculate current FPS
	var current_fps = Engine.get_frames_per_second()
	var frame_time_ms = (1.0 / current_fps) * 1000.0 if current_fps > 0 else 0.0

	# Update based on state
	match state:
		0:  # WARMUP
			_status_label.text = "Status: WARMING UP..."
			var progress = (warmup_elapsed / warmup_duration) * 100.0
			_progress_bar.value = progress
			_fps_label.text = "FPS: %.1f (not measured)" % current_fps
			_frame_time_label.text = "Frame Time: %.2f ms (warmup)" % frame_time_ms

			if show_detailed_stats and _details_label:
				_details_label.text = "Warmup: %.1fs / %.1fs\nStabilizing performance..." % [warmup_elapsed, warmup_duration]

		1:  # TESTING
			_status_label.text = "Status: BENCHMARKING..."
			var progress = (test_elapsed / test_duration) * 100.0
			_progress_bar.value = progress

			# Calculate stats from collected samples
			var samples = _controller._fps_samples
			var avg_fps = 0.0
			var min_fps = 9999.0
			var max_fps = 0.0

			if samples.size() > 0:
				for fps in samples:
					avg_fps += fps
					min_fps = min(min_fps, fps)
					max_fps = max(max_fps, fps)
				avg_fps /= samples.size()

			_fps_label.text = "Current FPS: %.1f" % current_fps
			_frame_time_label.text = "Frame Time: %.2f ms" % frame_time_ms

			if show_detailed_stats and _details_label:
				_details_label.text = (
					"Progress: %.1fs / %.1fs\n" % [test_elapsed, test_duration] +
					"Frames Captured: %d\n\n" % samples.size() +
					"Average FPS: %.1f\n" % avg_fps +
					"Min FPS: %.1f\n" % min_fps +
					"Max FPS: %.1f" % max_fps
				)

		2:  # COMPLETE
			_status_label.text = "Status: COMPLETE!"
			_status_label.add_theme_color_override("font_color", Color.GREEN)
			_progress_bar.value = 100
			_fps_label.text = "Benchmark Complete"
			_frame_time_label.text = "Processing results..."

			if show_detailed_stats and _details_label:
				_details_label.text = "Check console for full results.\nQuitting automatically..."
