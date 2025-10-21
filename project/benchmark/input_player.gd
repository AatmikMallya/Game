extends Node

## Input Player
## Replays recorded input sequences for consistent benchmarking

@export var playback_enabled: bool = false
@export var input_file: String = "user://benchmark_inputs.json"
@export var loop_playback: bool = false

# Playback state
var _events: Array = []
var _current_event_index: int = 0
var _playback_start_time: float = 0.0
var _playing: bool = false
var _duration: float = 0.0

# Simulated input state (for action queries)
var _action_states: Dictionary = {}

func _ready() -> void:
	if playback_enabled:
		load_and_start()

func load_and_start() -> void:
	if load_recording():
		start_playback()

func load_recording() -> bool:
	var file = FileAccess.open(input_file, FileAccess.READ)
	if not file:
		printerr("Failed to load input recording: %s" % input_file)
		return false

	var json_text = file.get_as_text()
	file.close()

	var json = JSON.new()
	var error = json.parse(json_text)
	if error != OK:
		printerr("Failed to parse input recording JSON: %s" % json.get_error_message())
		return false

	var data = json.data
	if not data.has("events"):
		printerr("Invalid recording format: missing 'events' field")
		return false

	_events = data["events"]
	_duration = data.get("duration", 0.0)

	print("=== Input Playback Loaded ===")
	print("Duration: %.2fs" % _duration)
	print("Events: %d" % _events.size())

	return true

func start_playback() -> void:
	_playing = true
	_current_event_index = 0
	_playback_start_time = Time.get_ticks_usec() / 1000000.0
	_action_states.clear()

	print("=== Input Playback Started ===")

func stop_playback() -> void:
	_playing = false
	print("=== Input Playback Stopped ===")

func _process(_delta: float) -> void:
	if not _playing or _events.is_empty():
		return

	var current_time = (Time.get_ticks_usec() / 1000000.0) - _playback_start_time

	# Process all events that should have happened by now
	while _current_event_index < _events.size():
		var event = _events[_current_event_index]
		var event_time = event["timestamp"]

		if event_time > current_time:
			break

		_process_event(event)
		_current_event_index += 1

	# Check if playback finished
	if _current_event_index >= _events.size():
		if loop_playback:
			print("Looping playback...")
			start_playback()
		else:
			stop_playback()

func _process_event(event: Dictionary) -> void:
	match event["type"]:
		"mouse_motion":
			var motion_event = InputEventMouseMotion.new()
			motion_event.relative = Vector2(event["relative_x"], event["relative_y"])
			Input.parse_input_event(motion_event)

		"action_pressed":
			var action_event = InputEventAction.new()
			action_event.action = event["action"]
			action_event.pressed = true
			Input.parse_input_event(action_event)
			_action_states[event["action"]] = true

		"action_released":
			var action_event = InputEventAction.new()
			action_event.action = event["action"]
			action_event.pressed = false
			Input.parse_input_event(action_event)
			_action_states[event["action"]] = false

		"start":
			pass  # Initial marker

		"stop":
			pass  # Final marker

# Helper function for other scripts to check if action is currently pressed during playback
func is_action_pressed(action: String) -> bool:
	if not _playing:
		return false
	return _action_states.get(action, false)

func get_playback_progress() -> float:
	if not _playing or _duration <= 0.0:
		return 0.0

	var current_time = (Time.get_ticks_usec() / 1000000.0) - _playback_start_time
	return clamp(current_time / _duration, 0.0, 1.0)
