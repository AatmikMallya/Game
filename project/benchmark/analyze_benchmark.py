#!/usr/bin/env python3
"""
Voxel Engine Benchmark Analyzer
Raw statistical analysis of per-frame CSV data
"""

import sys
import csv
import statistics
from pathlib import Path

def load_csv(filepath):
    """Load CSV data into list of dictionaries"""
    data = []
    with open(filepath, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            # Convert numeric fields
            numeric_row = {}
            for key, value in row.items():
                try:
                    numeric_row[key] = float(value)
                except ValueError:
                    numeric_row[key] = value
            data.append(numeric_row)
    return data

def analyze_frame_times(data):
    """Frame time statistics"""
    frame_times = [row['frame_time_ms'] for row in data]

    print("=" * 80)
    print("FRAME TIME STATISTICS")
    print("=" * 80)

    print(f"\nTotal frames: {len(frame_times)}")
    print(f"Mean: {statistics.mean(frame_times):.3f}ms")
    print(f"Median: {statistics.median(frame_times):.3f}ms")
    print(f"Std deviation: {statistics.stdev(frame_times):.3f}ms")
    print(f"Min: {min(frame_times):.3f}ms")
    print(f"Max: {max(frame_times):.3f}ms")

    sorted_times = sorted(frame_times)
    print(f"\nPercentiles:")
    print(f"  1%:  {sorted_times[int(len(sorted_times) * 0.01)]:.3f}ms")
    print(f"  5%:  {sorted_times[int(len(sorted_times) * 0.05)]:.3f}ms")
    print(f"  25%: {sorted_times[int(len(sorted_times) * 0.25)]:.3f}ms")
    print(f"  50%: {sorted_times[int(len(sorted_times) * 0.50)]:.3f}ms")
    print(f"  75%: {sorted_times[int(len(sorted_times) * 0.75)]:.3f}ms")
    print(f"  95%: {sorted_times[int(len(sorted_times) * 0.95)]:.3f}ms")
    print(f"  99%: {sorted_times[int(len(sorted_times) * 0.99)]:.3f}ms")

    print("\nDistribution by bucket:")
    buckets = [
        ("0-4ms", 0, 4),
        ("4-8ms", 4, 8),
        ("8-16ms", 8, 16),
        ("16-33ms", 16, 33),
        ("33+ms", 33, float('inf')),
    ]
    for name, low, high in buckets:
        count = len([ft for ft in frame_times if low <= ft < high])
        pct = count / len(frame_times) * 100
        print(f"  {name:10s}: {count:5d} frames ({pct:5.1f}%)")

def analyze_memory(data):
    """Memory usage statistics"""
    print("\n" + "=" * 80)
    print("MEMORY STATISTICS (MB)")
    print("=" * 80)

    mem_static = [row['memory_static'] / 1024 / 1024 for row in data]
    video_mem = [row['render_video_mem_used'] / 1024 / 1024 for row in data]
    texture_mem = [row['render_texture_mem_used'] / 1024 / 1024 for row in data]
    buffer_mem = [row['render_buffer_mem_used'] / 1024 / 1024 for row in data]

    print(f"\nStatic memory:")
    print(f"  Min:    {min(mem_static):.2f} MB")
    print(f"  Max:    {max(mem_static):.2f} MB")
    print(f"  Mean:   {statistics.mean(mem_static):.2f} MB")
    print(f"  Median: {statistics.median(mem_static):.2f} MB")
    print(f"  Delta:  {max(mem_static) - min(mem_static):.2f} MB")

    print(f"\nVideo memory (mean):")
    print(f"  Total:    {statistics.mean(video_mem):.2f} MB")
    print(f"  Textures: {statistics.mean(texture_mem):.2f} MB ({statistics.mean(texture_mem)/statistics.mean(video_mem)*100:.1f}%)")
    print(f"  Buffers:  {statistics.mean(buffer_mem):.2f} MB ({statistics.mean(buffer_mem)/statistics.mean(video_mem)*100:.1f}%)")

def analyze_rendering(data):
    """Rendering statistics"""
    print("\n" + "=" * 80)
    print("RENDERING STATISTICS")
    print("=" * 80)

    draw_calls = [row['render_total_draw_calls'] for row in data]
    objects = [row['render_total_objects'] for row in data]
    primitives = [row['render_total_primitives'] for row in data]

    print(f"\nDraw calls per frame:")
    print(f"  Min:  {int(min(draw_calls))}")
    print(f"  Max:  {int(max(draw_calls))}")
    print(f"  Mean: {statistics.mean(draw_calls):.2f}")

    print(f"\nObjects per frame:")
    print(f"  Min:  {int(min(objects))}")
    print(f"  Max:  {int(max(objects))}")
    print(f"  Mean: {statistics.mean(objects):.2f}")

    print(f"\nPrimitives per frame:")
    print(f"  Min:  {int(min(primitives))}")
    print(f"  Max:  {int(max(primitives))}")
    print(f"  Mean: {statistics.mean(primitives):.2f}")

def find_slowest_frames(data, n=10):
    """Slowest N frames by frame time"""
    print("\n" + "=" * 80)
    print(f"SLOWEST {n} FRAMES (by frame time)")
    print("=" * 80)

    sorted_data = sorted(data, key=lambda x: x['frame_time_ms'], reverse=True)

    print(f"\n{'Frame':<8} {'Time (ms)':<12} {'Delta (ms)':<12} {'Timestamp (s)':<15}")
    print("-" * 60)

    for frame in sorted_data[:n]:
        print(f"{int(frame['frame']):>6}   {frame['frame_time_ms']:>10.3f}   "
              f"{frame['delta']*1000:>10.3f}   {frame['timestamp']:>13.3f}")

def analyze_time_breakdown(data, n=20):
    """Per-frame time breakdown showing percentage spent in each stage"""
    print("\n" + "=" * 80)
    print(f"DETAILED PER-FRAME BREAKDOWN (first {n} frames)")
    print("=" * 80)
    print("\nShowing CPU-measured timings for each rendering step.\n")

    # Check if we have GPU timing data (newer profiling)
    has_gpu_timing = 'gpu_voxel_camera_raymarching' in data[0]

    if has_gpu_timing:
        print(f"{'Frame':<6} {'Total':<8} {'GPU-Liquid':<12} {'GPU-Freeze':<12} {'GPU-Clean':<12} {'GPU-March':<12}")
        print(f"{'':6} {'(ms)':<8} {'(ms %)':<12} {'(ms %)':<12} {'(ms %)':<12} {'(ms %)':<12}")
        print("-" * 80)

        for i in range(min(n, len(data))):
            frame = data[i]
            total_ms = frame['frame_time_ms']

            # Get GPU timings (already in ms)
            gpu_liquid_ms = frame.get('gpu_voxel_world_sim_liquid', 0)
            gpu_freeze_ms = frame.get('gpu_voxel_world_sim_freeze', 0)
            gpu_cleanup_ms = frame.get('gpu_voxel_world_sim_cleanup', 0)
            gpu_raymarch_ms = frame.get('gpu_voxel_camera_raymarching', 0)

            # Calculate percentages
            gpu_liquid_pct = (gpu_liquid_ms / total_ms * 100) if total_ms > 0 else 0
            gpu_freeze_pct = (gpu_freeze_ms / total_ms * 100) if total_ms > 0 else 0
            gpu_cleanup_pct = (gpu_cleanup_ms / total_ms * 100) if total_ms > 0 else 0
            gpu_raymarch_pct = (gpu_raymarch_ms / total_ms * 100) if total_ms > 0 else 0

            print(f"{int(frame['frame']):>5}  {total_ms:>7.3f}  "
                  f"{gpu_liquid_ms:>6.3f} {gpu_liquid_pct:>4.1f}  "
                  f"{gpu_freeze_ms:>6.3f} {gpu_freeze_pct:>4.1f}  "
                  f"{gpu_cleanup_ms:>6.3f} {gpu_cleanup_pct:>4.1f}  "
                  f"{gpu_raymarch_ms:>6.3f} {gpu_raymarch_pct:>4.1f}")

        # Aggregate statistics
        print("\n" + "=" * 80)
        print("AGGREGATE GPU TIMING BREAKDOWN (all frames)")
        print("=" * 80)

        total_frame_time = sum(f['frame_time_ms'] for f in data)
        total_gpu_liquid = sum(f.get('gpu_voxel_world_sim_liquid', 0) for f in data)
        total_gpu_freeze = sum(f.get('gpu_voxel_world_sim_freeze', 0) for f in data)
        total_gpu_cleanup = sum(f.get('gpu_voxel_world_sim_cleanup', 0) for f in data)
        total_gpu_raymarch = sum(f.get('gpu_voxel_camera_raymarching', 0) for f in data)

        # Total measured GPU time
        measured_gpu_total = total_gpu_liquid + total_gpu_freeze + total_gpu_cleanup + total_gpu_raymarch

        # CPU overhead (frame time minus measured GPU time)
        cpu_overhead = total_frame_time - measured_gpu_total

        print(f"\nTotal frame time: {total_frame_time:.2f}ms")
        print(f"\nGPU Compute Breakdown:")
        print(f"  GPU - Liquid Sim:      {total_gpu_liquid:>10.2f}ms ({total_gpu_liquid/total_frame_time*100:>5.1f}%)")
        print(f"  GPU - Freeze Sim:      {total_gpu_freeze:>10.2f}ms ({total_gpu_freeze/total_frame_time*100:>5.1f}%)")
        print(f"  GPU - Cleanup Sim:     {total_gpu_cleanup:>10.2f}ms ({total_gpu_cleanup/total_frame_time*100:>5.1f}%)")
        print(f"  GPU - Voxel Raymarch:  {total_gpu_raymarch:>10.2f}ms ({total_gpu_raymarch/total_frame_time*100:>5.1f}%)")
        print(f"  ─────────────────────────────────────────────────")
        print(f"  Total GPU Time:        {measured_gpu_total:>10.2f}ms ({measured_gpu_total/total_frame_time*100:>5.1f}%)")
        print(f"  CPU Overhead/Sync:     {cpu_overhead:>10.2f}ms ({cpu_overhead/total_frame_time*100:>5.1f}%)")

        # Add CPU timing breakdown if available
        if 'voxel_camera_raymarching' in data[0]:
            total_vw_update = sum(f.get('voxel_world_total_update', 0) for f in data) * 1000  # Convert to ms
            total_vw_liquid = sum(f.get('voxel_world_sim_liquid', 0) for f in data) * 1000
            total_vw_freeze = sum(f.get('voxel_world_sim_freeze', 0) for f in data) * 1000
            total_vw_cleanup = sum(f.get('voxel_world_sim_cleanup', 0) for f in data) * 1000
            total_vw_collision = sum(f.get('voxel_world_collision', 0) for f in data) * 1000

            total_cam_render = sum(f.get('voxel_camera_total_render', 0) for f in data) * 1000
            total_cam_update = sum(f.get('voxel_camera_update', 0) for f in data) * 1000
            total_cam_raymarch = sum(f.get('voxel_camera_raymarching', 0) for f in data) * 1000

            total_measured_cpu = total_vw_update + total_cam_render

            print(f"\n{'='*80}")
            print("CPU TIMING BREAKDOWN (measured, not GPU time)")
            print(f"{'='*80}\n")

            print("Voxel World:")
            print(f"  Total Update:       {total_vw_update:>10.2f}ms ({total_vw_update/total_frame_time*100:>5.1f}%)")
            print(f"    Liquid Sim:       {total_vw_liquid:>10.2f}ms ({total_vw_liquid/total_frame_time*100:>5.1f}%)")
            print(f"    Freeze Sim:       {total_vw_freeze:>10.2f}ms ({total_vw_freeze/total_frame_time*100:>5.1f}%)")
            print(f"    Cleanup Sim:      {total_vw_cleanup:>10.2f}ms ({total_vw_cleanup/total_frame_time*100:>5.1f}%)")
            print(f"    Collision:        {total_vw_collision:>10.2f}ms ({total_vw_collision/total_frame_time*100:>5.1f}%)")

            print(f"\nVoxel Camera:")
            print(f"  Total Render:       {total_cam_render:>10.2f}ms ({total_cam_render/total_frame_time*100:>5.1f}%)")
            print(f"    Camera Update:    {total_cam_update:>10.2f}ms ({total_cam_update/total_frame_time*100:>5.1f}%)")
            print(f"    Raymarching:      {total_cam_raymarch:>10.2f}ms ({total_cam_raymarch/total_frame_time*100:>5.1f}%)")

            print(f"\n  ─────────────────────────────────────────────────")
            print(f"  Total Measured:     {total_measured_cpu:>10.2f}ms ({total_measured_cpu/total_frame_time*100:>5.1f}%)")
            print(f"  Unmeasured:         {total_frame_time - total_measured_cpu:>10.2f}ms ({(total_frame_time-total_measured_cpu)/total_frame_time*100:>5.1f}%)")
    else:
        print("No detailed profiling data available in this benchmark.")
        print("Using fallback Godot Performance monitors.\n")

        print(f"{'Frame':<8} {'Total':<10} {'CPU Work':<14} {'GPU/Other':<14}")
        print(f"{'':8} {'(ms)':<10} {'ms (%)':<14} {'ms (%)':<14}")
        print("-" * 70)

        # Calculate deltas between frames for cumulative metrics
        for i in range(1, min(n+1, len(data))):
            prev_frame = data[i-1]
            curr_frame = data[i]

            total_ms = curr_frame['frame_time_ms']

            # Delta in cumulative times
            process_delta = (curr_frame['time_process'] - prev_frame['time_process']) * 1000
            physics_delta = (curr_frame['time_physics_process'] - prev_frame['time_physics_process']) * 1000
            nav_delta = (curr_frame['time_navigation_process'] - prev_frame['time_navigation_process']) * 1000

            cpu_work = process_delta + physics_delta + nav_delta
            gpu_other = total_ms - cpu_work

            cpu_pct = (cpu_work / total_ms * 100) if total_ms > 0 else 0
            gpu_pct = (gpu_other / total_ms * 100) if total_ms > 0 else 0

            print(f"{int(curr_frame['frame']):>6}   {total_ms:>8.3f}   "
                  f"{cpu_work:>7.3f} ({cpu_pct:>5.1f}%)   "
                  f"{gpu_other:>7.3f} ({gpu_pct:>5.1f}%)")

def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_benchmark.py <benchmark_csv_file>")
        sys.exit(1)

    filepath = Path(sys.argv[1])
    if not filepath.exists():
        print(f"Error: File not found: {filepath}")
        sys.exit(1)

    print(f"\nAnalyzing: {filepath.name}")
    print(f"Size: {filepath.stat().st_size / 1024:.1f} KB\n")

    data = load_csv(filepath)

    analyze_frame_times(data)
    analyze_memory(data)
    analyze_rendering(data)
    analyze_time_breakdown(data, n=30)
    find_slowest_frames(data, n=15)

    print("\n" + "=" * 80)

if __name__ == "__main__":
    main()
