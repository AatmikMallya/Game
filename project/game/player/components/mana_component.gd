extends Node
class_name ManaComponent

## Mana management component with automatic regeneration

signal mana_changed(current: float, maximum: float)
signal mana_depleted()
signal mana_used(amount: float)

@export var max_mana: float = 100.0
@export var mana_regen_rate: float = 10.0  # Mana per second

var current_mana: float

func _ready():
	current_mana = max_mana
	mana_changed.emit(current_mana, max_mana)

func _process(delta: float):
	# Automatic mana regeneration
	if current_mana < max_mana:
		regenerate(mana_regen_rate * delta)

func use_mana(amount: float) -> bool:
	if current_mana >= amount:
		current_mana -= amount
		mana_used.emit(amount)
		mana_changed.emit(current_mana, max_mana)

		if current_mana == 0:
			mana_depleted.emit()

		return true
	return false

func regenerate(amount: float) -> void:
	if current_mana >= max_mana:
		return

	current_mana = min(max_mana, current_mana + amount)
	mana_changed.emit(current_mana, max_mana)

func has_mana(amount: float) -> bool:
	return current_mana >= amount

func get_mana_percent() -> float:
	if max_mana == 0:
		return 0.0
	return current_mana / max_mana

func set_max_mana(new_max: float) -> void:
	max_mana = new_max
	current_mana = min(current_mana, max_mana)
	mana_changed.emit(current_mana, max_mana)

func reset_mana() -> void:
	current_mana = max_mana
	mana_changed.emit(current_mana, max_mana)

func add_mana(amount: float) -> void:
	regenerate(amount)
