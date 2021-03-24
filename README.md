# Installation - Linux
1. Install SFML: https://www.sfml-dev.org/tutorials/2.5/#getting-started
    * debian: `sudo apt-get install libsfml-dev`
2. Clone this repo
    * `git clone https://github.com/Honzurka/Bambulanci`
3. Clone submodules: https://github.com/ocornut/imgui, https://github.com/eliasdaler/imgui-sfml
    * `git submodule init`
    * `git submodule update`
4. Build project
    * `cmake .`
    * `make`
5. Run simulation
    * `./magnety`

# User Guide
## Buttons
- `Start`/`Stop` - Starts/stops simulation.
- `Reset` - Resets simulation.

## Visual parameters
Require resetting simulation to have effect.
- `radius` - Size of magnets.
- `magnet count` - Number of magnets in simulation. Could be lowered if given number of magnets won't fit on screen.

## Movement parameters
- `movCoef` - Scales magnets movement. Is limited by inertia.
- `rotationCoef` - Scales magnets rotation.
- `inertia` - How much energy should be conserved from previous movement.
- `force const` - Artifical increase of both movement and rotation.
- `attract lim` - When should be attraction force of nearby magnets ignored. 1 = never, 0 = always.

# Programmer Documentation
> FPS is limited to `60` in `main()`.
> 
> In each frame `sim.Step()` is called.
> 
>> Step Computes magnets movement and rotation by calling `AlterForces()` on each pair.
> 
>>> Attract/repel rotation is computed separatedly in `RotateAttract`, `RotateRepel`.
>
>> Coefficients (`inertia`, `movCoef`, `rotationCoef`) are then applied on computed forces by calling `ApplyCoeffs()`.
> 
>> Magnets are moved by `MoveMagnets()`.
> 
>>> Handles collisions by propotionally decreasing movement vector in `ResolveMagCollision`, `ResolveWinCollisionA` and finally moves magnets.





