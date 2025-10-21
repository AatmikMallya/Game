extends ProgressBar

## Simple mana bar that displays current/max mana

@export var show_text: bool = true

func _ready():
	# Set up visual style
	set_min(0)

func set_mana(current: float, maximum: float) -> void:
	max_value = maximum
	value = current

	# Update text if enabled (similar to health bar)
	if show_text:
		var percent = 0
		if maximum > 0:
			percent = int((current / maximum) * 100)

func _process(_delta):
	# Smooth visual transition (optional)
	pass
