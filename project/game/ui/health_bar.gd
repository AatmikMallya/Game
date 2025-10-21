extends ProgressBar

## Simple health bar that displays current/max health

@export var show_text: bool = true

func _ready():
	# Set up visual style
	set_min(0)

func set_health(current: int, maximum: int) -> void:
	max_value = maximum
	value = current

	# Update text if enabled
	if show_text:
		var percent = 0
		if maximum > 0:
			percent = int((float(current) / float(maximum)) * 100)
		# Note: ProgressBar doesn't have built-in text, you'd add a Label child
		# For now, this just updates the bar value

func _process(_delta):
	# Smooth visual transition (optional)
	pass
