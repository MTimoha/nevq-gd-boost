# GD Boost

System-level performance tuning for Geometry Dash on Windows.

It changes **nothing visual**: no particles, shaders, effects or objects are touched.
Everything it does happens at the operating-system level:

- **Force high-performance GPU**: makes Windows run the game on your fastest GPU (laptops, integrated + dedicated setups). Applies from the next launch.
- **Process priority**: raises CPU scheduling priority of the game.
- **Disable power throttling**: opts out of Windows Efficiency Mode (EcoQoS).
- **1 ms timer resolution**: steadier frame pacing.
- **Boost render/logic thread**: higher thread priority and the MMCSS "Games" profile.

All options can be switched off in the mod settings. Restart the game after changing them.

This mod does not rewrite GD's rendering or level loading. For that, see projects like Bismuth and AlgebraDash.
