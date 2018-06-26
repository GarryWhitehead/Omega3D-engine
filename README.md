# Omega3D-engine
A 3D-engine that utilises the Vulkan garphics API and entity component mechanics. 

The primary idea behinbd this engine is to allow the creation of interactive 3D demos and user-defined scripted playbacks. 
The idea is to experiment with different graphical algorthims and produce the most realistic, physically accurate 
graphical representations whilst still maintianing the real-time element. 

This is an on-going project and so far the following features have been implemented :

- Deferred shader pipeline
- Terrain generation using an imported heightmap
- Ocean rendering using the fast fourier transform alogrithim - evertything calculated on the GPU
- irradiance-based lighting model using a cube-map for environmental mapping
- PBR texture support - now nearly bug free!
- shadow mapping (need to finish adding)
- custom model format aloowing multiple models and materials to be imported into the engine - this uses an external program 
to convert .obj and .dae files (more model formats are planned)
- GUI - using the dear imGUI library - basic settings can be adjusted at the moment - more to be added
- post processing effects - only fog implemeneted at the moment - needs more work

And the immediate future plans (some nearly complete....):

- some lighting issues to address
- improve post-processing - volumetric fog, ambient occlusion
- cross platform support
- finish adding shadows
- finish animation support 
- Improve the ECS system - allow creation/deletion of objects, etc.
- reflection probes
- surface light-scattering
- MSAA

