# Voxel Engine Automated Benchmark System

Automated performance evaluation harness for the microvoxel simulation engine.

## Quick Start

### macOS/Linux
```bash
./run_benchmark.sh
```

### Windows
```cmd
run_benchmark.bat
```

This will:
1. Compile the GDExtension (if needed)
2. Load the benchmark scene
3. Run a 10-second performance test
4. Output detailed statistics
5. Save results as JSON

---

## Features

- **Automated execution** - One command runs the entire benchmark
- **Warmup period** - 3 seconds to stabilize before measurement
- **Comprehensive statistics** - FPS, frame times, percentiles
- **Real-time display** - See performance metrics during the test
- **JSON output** - Machine-readable results for tracking over time
- **Configurable** - Adjust duration, world size, and more

---

## Output

### Console Summary
```
=== Performance Summary ===
Frames Captured: 584
Test Duration: 10.00s

FPS:
  Average: 58.4
  Minimum: 42.1
  Maximum: 61.2
  1%ile:   43.2
  5%ile:   51.8
  50%ile:  59.1
  95%ile:  60.8
  99%ile:  61.0

Frame Time (ms):
  Average: 17.12
  Std Dev: 2.43
  1%ile:   16.34
  99%ile:  23.26

Voxel World:
  Brick Map: 32x32x32
  Voxels: 256x256x256
  Scale: 0.250
  Simulation: Disabled
```

### JSON Results
Saved to: `~/Library/Application Support/Godot/app_userdata/game/benchmark_results/`

```json
{
  "timestamp": "2025-10-14T18:30:00",
  "config": {
    "resolution": [1920, 1080],
    "voxel_world": {
      "brick_map_size": "32x32x32",
      "voxel_scale": 0.25
    }
  },
  "results": {
    "fps": {
      "average": 58.4,
      "percentiles": {
        "p1": 43.2,
        "p99": 61.0
      }
    }
  }
}
```

---

## Configuration

### Benchmark Controller Settings
Edit `project/benchmark/benchmark.tscn` or modify exports in `benchmark_controller.gd`:

```gdscript
@export var warmup_duration: float = 3.0  # Warmup time
@export var test_duration: float = 10.0  # Benchmark duration
@export var enable_console_output: bool = true
@export var enable_json_output: bool = true
```

### World Size
Modify the VoxelWorld node in `benchmark.tscn`:

```gdscript
brick_map_size = Vector3i(16, 16, 16)  # Smaller world
brick_map_size = Vector3i(64, 64, 64)  # Larger world
```

### Camera Position
Edit the VoxelCamera transform in the scene to test different viewpoints.

---

## Command-Line Options

### macOS/Linux
```bash
./run_benchmark.sh [OPTIONS]

Options:
  --skip-build       Skip compilation, run benchmark only
  --rebuild          Clean and rebuild extension
  --release          Use release build instead of debug
  --platform=NAME    Target platform (default: macos)
  --jobs=N           Parallel compilation jobs (default: 9)
  --help             Show this help message
```

### Windows
```cmd
run_benchmark.bat [OPTIONS]

Options:
  --skip-build       Skip compilation, run benchmark only
  --rebuild          Clean and rebuild extension
  --release          Use release build instead of debug
  --jobs N           Parallel compilation jobs (default: 9)
  --help             Show this help message
```

### Examples
```bash
# Quick run (skip rebuild)
./run_benchmark.sh --skip-build

# Full clean rebuild
./run_benchmark.sh --rebuild

# Release build for max performance
./run_benchmark.sh --release

# Faster compilation
./run_benchmark.sh --jobs=16
```

---

## Environment Variables

### GODOT_BIN
Override the Godot executable path:

```bash
# macOS/Linux
export GODOT_BIN=/path/to/custom/godot
./run_benchmark.sh

# Windows
set GODOT_BIN=C:\Path\To\Godot.exe
run_benchmark.bat
```

---

## Use Cases

### 1. Performance Tracking
Run benchmarks after each optimization to measure improvement:

```bash
# Before optimization
./run_benchmark.sh --release > before.txt

# After optimization
./run_benchmark.sh --release > after.txt

# Compare results
diff before.txt after.txt
```

### 2. Configuration Testing
Test different world sizes:

1. Edit `benchmark.tscn` → Set `brick_map_size`
2. Run benchmark
3. Compare FPS vs. world size

### 3. Regression Detection
Commit baseline results to git:

```bash
./run_benchmark.sh --release
cp ~/Library/.../benchmark_results/latest.json baseline.json
git add baseline.json
```

Later, compare against baseline to detect regressions.

---

## Interpreting Results

### FPS Metrics
- **Average FPS**: Overall performance indicator
- **Minimum FPS**: Worst-case stuttering
- **1%ile/5%ile**: Low percentiles show stuttering frequency
- **99%ile**: Best-case performance

### Frame Time Metrics
- **Average**: Inverse of avg FPS
- **Std Dev**: Lower = more stable framerate
- **99%ile**: Maximum frame time (for detecting spikes)

### What to Look For
- **High std dev** → Inconsistent performance (investigate!)
- **Low 1%ile FPS** → Severe stuttering (check simulation)
- **Avg < 30 FPS** → Workload too heavy

---

## Customization

### Add Custom Metrics
Edit `benchmark_controller.gd` → `_compute_statistics()`:

```gdscript
# Track custom data
var ray_steps_per_frame = []

# In _process_testing():
ray_steps_per_frame.append(get_custom_metric())

# In results:
"custom_metrics": {
    "avg_ray_steps": average(ray_steps_per_frame)
}
```

### Modify Camera Path
Edit `benchmark.tscn` → VoxelCamera → Add script:

```gdscript
var waypoints = [Vector3(0, 50, 0), Vector3(100, 50, 100)]
var t = 0.0

func _process(delta):
    t += delta * 0.1
    global_position = interpolate_waypoints(t)
```

### Change Test Scenario
Duplicate `benchmark.tscn` → Create variants:
- `benchmark_small.tscn` (16³ world)
- `benchmark_large.tscn` (64³ world)
- `benchmark_simulation.tscn` (liquids enabled)

---

## Troubleshooting

### "Godot not found"
Set `GODOT_BIN` environment variable or install to default path:
- macOS: `/Applications/Godot.app`
- Windows: `C:\Program Files\Godot\Godot.exe`

### "Build failed"
Check that SCons is installed:
```bash
python3 -m pip install scons
```

### "No results output"
Check the user data directory:
```bash
# macOS
~/Library/Application Support/Godot/app_userdata/game/benchmark_results/

# Linux
~/.local/share/godot/app_userdata/game/benchmark_results/

# Windows
%APPDATA%\Godot\app_userdata\game\benchmark_results\
```

### "Benchmark hangs"
Press Ctrl+C to abort. Check console for errors.

---

## Future Enhancements

See the main project plan for upcoming features:
- Camera flight paths for reproducible tests
- GPU profiling integration
- Automated comparison with historical data
- Stress testing modes
- CI/CD integration

---

## Architecture

```
benchmark.tscn
├── BenchmarkController (benchmark_controller.gd)
│   ├── Warmup phase (3s)
│   ├── Testing phase (10s)
│   ├── Statistics computation
│   └── Results output (console + JSON)
├── VoxelWorld
│   ├── Fixed 32³ brick map
│   └── Shader-based generation
├── VoxelCamera
│   ├── Fixed position (no player)
│   ├── TextureRect output
│   └── BenchmarkPanel overlay
└── WorldEnvironment
```

---

## Performance Baseline

Typical results on M1 MacBook Pro (2021):
- **Average FPS**: 55-65 (32³ world, 1920×1080)
- **Frame Time**: 15-18ms
- **1%ile FPS**: 45-50

Your results will vary based on:
- Hardware (GPU, CPU, RAM)
- World size
- Voxel density
- Simulation enabled/disabled

Use these benchmarks to track **relative** improvements over time!
