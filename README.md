# VulkanSceneGraph Train Simulator

This should eventually be a version of my rail marine simulator that uses
VulkanSceneGraph.  It can use Microsoft Train Simulator (MSTS)
routes and rolling stock, but it does not attempt to duplicate MSTS behavoir
or support all MSTS content.  There is only limited support for MSTS
activity, wagon and engine files.

The current version runs on Pop!OS 22.04.
Compiling on Windows has never been attempted.

MIT License

## Compiling

cmake .
make

Dependencies include: VSG, openal, plib, zlib and microhttpd.
