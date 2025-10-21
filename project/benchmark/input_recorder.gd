extends Node

## Input Recorder
## Captures all player inputs with timestamps for benchmark replay

@export var recording_enabled: bool = true
@export var output_file: String = "user://benchmark_inputs.json"

# Recording state
var _recording_data: Array[Dictionary] = []
var _start_time: float = 0.0
var _recording: bool = false

# Tracked actions
const TRACKED_ACTIONS = [
	"move_forward", "move_backward", "move_left", "move_right",
	"move_up", "move_down",
	"left_click", "right_click",
	"change_player_mode",
	"scroll_up", "scroll_down",
	"ui_accept"
]

func _ready() -> void:
	if recording_enabled:
		start_recording()

func start_recording() -> void:
	print("=== Input Recorder Started ===")
	_recording = true
	_recording_data.clear()
	_start_time = Time.get_ticks_usec() / 1000000.0

	# Record initial state
	_record_event({
		"type": "start",
		"timestamp": 0.0
	})

func stop_recording() -> void:
	if not _recording:
		return

	_recording = false

	# Record final state
	var elapsed = (Time.get_ticks_usec() / 1000000.0) - _start_time
	_record_event({
		"type": "stop",
		"timestamp": elapsed
	})

	save_recording()
	print("=== Input Recording Stopped ===")
	print("Captured %d input events over %.2fs" % [_recording_data.size(), elapsed])

func _input(event: InputEvent) -> void:
	if not _recording:
		return

	var timestamp = (Time.get_ticks_usec() / 1000000.0) - _start_time

	# Record mouse motion
	if event is InputEventMouseMotion:
		_record_event({
			"type": "mouse_motion",
			"timestamp": timestamp,
			"relative_x": event.relative.x,
			"relative_y": event.relative.y
		})

	# Record action presses/releases
	for action in TRACKED_ACTIONS:
		if event.is_action_pressed(action):
			_record_event({
				"type": "action_pressed",
				"timestamp": timestamp,
				"action": action
			})
		elif event.is_action_released(action):
			_record_event({
				"type": "action_released",
				"timestamp": timestamp,
				"action": action
			})

func _record_event(event_data: Dictionary) -> void:
	_recording_data.append(event_data)

func save_recording() -> void:
	var data = {
		"version": 1,
		"duration": _recording_data[-1]["timestamp"] if _recording_data.size() > 0 else 0.0,
		"event_count": _recording_data.size(),
		"events": _recording_data
	}

	var json_string = JSON.stringify(data, "  ")

	var file = FileAccess.open(output_file, FileAccess.WRITE)
	if file:
		file.store_string(json_string)
		file.close()
		print("Recording saved to: %s" % output_file)
	else:
		printerr("Failed to save recording to: %s" % output_file)

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		stop_recording()
