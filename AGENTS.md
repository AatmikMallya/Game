This a Godot 4.5 project. We are building a 3d microvoxel cube world game, with a visual style inspired by teardown, and gameplay inspired by noita. The goal is a built a large and deep world with a heavy emphasis on a deep magic combat system and simulation, like noita. Thus, we are using DDA ray marching for rendering along with some other rendering tricks.

GUIDELINES:
1. We do have a benchmark that plays recorded inputs on a test scene. We can use this when making any performance targeted improvement: /Applications/Godot.app/Contents/MacOS/Godot --path project res://benchmark/benchmark_replay.tscn
2. the following aliases are available for building our C++ code: alias vgrel='cd "$VG_REPO" && scons platform=macos arch=arm64 target=template_release -j8'
alias vgrelfirst='cd "$VG_REPO" && scons platform=macos arch=arm64 target=template_release generate_bindings=yes -j8'
alias vgrelrun='cd "$VG_PROJ" && /Applications/Godot.app/Contents/MacOS/Godot --headless --path . --export-release "macOS" ../build/Game.app && open ../build/Game.app'

alias vgfirst='cd ~/Desktop/GDVoxelPlayground && scons platform=macos arch=arm64 target=template_debug generate_bindings=yes -j8 && /Applications/Godot.app/Contents/MacOS/Godot --path ~/Desktop/GDVoxelPlayground/project'
alias vg='cd ~/Desktop/GDVoxelPlayground && scons platform=macos arch=arm64 target=template_debug -j8 && /Applications/Godot.app/Contents/MacOS/Godot --path ~/Desktop/GDVoxelPlayground/project'

alias vgbuild='cd ~/Desktop/GDVoxelPlayground && scons platform=macos arch=arm64 target=template_debug -j8'


