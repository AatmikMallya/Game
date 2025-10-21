extends Control

## Main HUD that connects to player components and updates UI

@export var player: Node3D

@onready var health_bar = $VBoxContainer/HealthBar
@onready var mana_bar = $VBoxContainer/ManaBar

func _ready():
	# Find player if not assigned
	if not player:
		player = get_tree().get_first_node_in_group("player")

	if not player:
		push_error("HUD: Player not found!")
		return

	# Connect to player components
	_connect_to_health_component()
	_connect_to_mana_component()

func _connect_to_health_component():
	var health_comp = player.get_node_or_null("HealthComponent")
	if health_comp:
		health_comp.health_changed.connect(_on_health_changed)
		# Initialize with current values
		if health_bar:
			health_bar.set_health(health_comp.current_health, health_comp.max_health)
	else:
		push_warning("HUD: Player has no HealthComponent")

func _connect_to_mana_component():
	var mana_comp = player.get_node_or_null("ManaComponent")
	if mana_comp:
		mana_comp.mana_changed.connect(_on_mana_changed)
		# Initialize with current values
		if mana_bar:
			mana_bar.set_mana(mana_comp.current_mana, mana_comp.max_mana)
	else:
		push_warning("HUD: Player has no ManaComponent")

func _on_health_changed(current: int, maximum: int):
	if health_bar:
		health_bar.set_health(current, maximum)

func _on_mana_changed(current: float, maximum: float):
	if mana_bar:
		mana_bar.set_mana(current, maximum)
