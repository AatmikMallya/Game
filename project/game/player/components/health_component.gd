extends Node
class_name HealthComponent

## Reusable health management component
## Attach to any entity (player, enemy) that needs health

signal health_changed(current: int, maximum: int)
signal died()
signal damage_taken(amount: int)
signal healed(amount: int)

@export var max_health: int = 100
var current_health: int

func _ready():
	current_health = max_health
	health_changed.emit(current_health, max_health)

func take_damage(amount: int) -> void:
	if current_health <= 0:
		return  # Already dead

	var actual_damage = min(amount, current_health)
	current_health = max(0, current_health - amount)

	damage_taken.emit(actual_damage)
	health_changed.emit(current_health, max_health)

	if current_health == 0:
		died.emit()

func heal(amount: int) -> void:
	if current_health <= 0:
		return  # Can't heal if dead

	var actual_heal = min(amount, max_health - current_health)
	current_health = min(max_health, current_health + amount)

	if actual_heal > 0:
		healed.emit(actual_heal)
		health_changed.emit(current_health, max_health)

func is_alive() -> bool:
	return current_health > 0

func get_health_percent() -> float:
	if max_health == 0:
		return 0.0
	return float(current_health) / float(max_health)

func set_max_health(new_max: int) -> void:
	max_health = new_max
	current_health = min(current_health, max_health)
	health_changed.emit(current_health, max_health)

func reset_health() -> void:
	current_health = max_health
	health_changed.emit(current_health, max_health)
