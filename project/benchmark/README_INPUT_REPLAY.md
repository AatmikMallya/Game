# Input Recording and Replay System

This system allows you to record gameplay sessions and replay them for consistent, reproducible benchmarking.

## Quick Start

### 1. Record a Gameplay Session

Run the demo scene with recording enabled:

```bash
/Applications/Godot.app/Contents/MacOS/Godot --path project res://demo/demo_record.tscn
```

Play the demo normally:
- **WASD**: Move around
- **Mouse**: Look around
- **Space/Shift**: Move up/down (in flying mode)
- **Left Click**: Place voxels
- **Right Click**: Remove voxels
- **1-4 Keys**: Change voxel type (inventory selection)
- **F**: Toggle flying/walking mode

When you're done (or after ~20-30 seconds), close the window. The inputs will be automatically saved to:
- `~/Library/Application Support/Godot/app_userdata/Game/benchmark_inputs.json`

### 2. Run Benchmark with Recorded Inputs

```bash
/Applications/Godot.app/Contents/MacOS/Godot --path project res://benchmark/benchmark_replay.tscn
```

The benchmark will:
1. Load your recorded inputs
2. Warmup for 3 seconds
3. Replay your inputs for 10 seconds while benchmarking
4. Save detailed performance data to `user://benchmark_results/`

### 3. Analyze Results

```bash
cd project/benchmark
python3 analyze_benchmark.py "/Users/aatmikmallya/Library/Application Support/Godot/app_userdata/Game/benchmark_results/benchmark_YYYY-MM-DD_HH-MM-SS_detailed.csv"
```

## Files

- **input_recorder.gd**: Captures all player inputs with timestamps
- **input_player.gd**: Replays recorded inputs
- **demo_record.tscn**: Demo scene with input recording enabled
- **benchmark_replay.tscn**: Benchmark scene with input playback and profiling

## Recording Format

Inputs are saved as JSON with the following structure:

```json
{
  "version": 1,
  "duration": 25.3,
  "event_count": 1042,
  "events": [
    {
      "type": "mouse_motion",
      "timestamp": 0.523,
      "relative_x": -5.2,
      "relative_y": 2.1
    },
    {
      "type": "action_pressed",
      "timestamp": 1.245,
      "action": "move_forward"
    },
    ...
  ]
}
```

## Benefits

1. **Reproducibility**: Same inputs every run = comparable results
2. **Real Workloads**: Captures actual gameplay patterns (movement, voxel editing, camera rotation)
3. **Consistency**: Eliminates human variability between benchmark runs
4. **Flexibility**: Record different scenarios (heavy editing, flying, combat, etc.)

## Advanced Usage

### Record Different Scenarios

Create specialized recordings for different workloads:
- **stress_test_inputs.json**: Rapid voxel editing, lots of water/lava placement
- **flythrough_inputs.json**: Smooth camera movement through the world
- **combat_inputs.json**: Fast movement and camera rotation

Change the output file in `demo_record.tscn` or via export:

```gdscript
@export var output_file: String = "user://stress_test_inputs.json"
```

### Loop Playback

For longer benchmarks, enable looping in `benchmark_replay.tscn`:

```gdscript
@export var loop_playback: bool = true
```

### Compare Different Worlds

Use the same recorded inputs with different world sizes:
- 32x32x32 bricks (baseline)
- 64x64x64 bricks (stress test)
- Different terrain types

This isolates the impact of world complexity from player behavior.
