# VulkanSceneGraph Train Simulator

This is a version of my rail marine simulator (rmsim) that uses VulkanSceneGraph.
It can use Microsoft Train Simulator (MSTS) routes and rolling stock,
but it does not attempt to duplicate MSTS behavoir or support all MSTS content.
There is limited support for MSTS activity, wagon and engine files and some Open Rails specific extensions.
The rmsim ship simulation features have not been included.

The current version runs on Pop!OS 24.04.
Compiling on Windows has never been attempted.

MIT License

## Compiling

cmake .
make

Dependencies include: VSG, openal, plib and zlib.

## Usage

bin/vsgts *[rmsim_startupFile* *[options]]*

The startup file will be used to initialize the simulation if specified.
The format is the same as recognized by rmsim.
If a startup file is not specified, the sim will prompt for an MSTS route and activity.
In this case the MSTSDIRS environment variable must contain a colon separated list of
MSTS installation directories.
See controls.md file for simulation controls.
