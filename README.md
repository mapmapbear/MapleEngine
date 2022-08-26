
# Introduction 

### ***Maple-Engine is my own engine for leaning mordern engine and rendering technique.***
                                                                                                                                                                       
                                                                                                                                                                       
![](https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/MapleEngine.png)

### Path Tracing (Hardware)

![image](https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/PathTrace.png)

### Voxel Cone Tracing

![image](https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/VXGI.png)

### Dynamic Diffuse Global Illumination with Raytracing Acceleration

![image](https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/DDGI.png)

### Ray Tracing Soft Shadow (with SVGF)

![image](https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/Soft-Shadow.png)


### Realtime Volumetric Cloud 

![](https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/cloud.png)

### Atmospheric Scattering
![](https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/Atmosphere.png)

### PBR
![image](https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/PBR.png)

### Light Propagation Volumes

![image](https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/LPV_GI.png)

## Current Features
- Path Trace
- OpenGL/Vulkan backends 
- Entity-component system( Based on entt )
- PBR/IBL
- Global Illumination(Light Propagation Volumes/Voxel Cone Tracing/DDGI)
- Directional lights + Cascaded shadow maps
- Soft shadows (PCF)
- Ray traced shadows (Spatiotemporal Variance-Guided Filtering)
- Screen Space Ambient Occlusion (SSAO)
- Screen Space Reflections(SSR)
- Lua Scripting
- Skinned Mesh
- C# Scripting (Mono)
- Ray-marched volumetric lighting
- Atmospheric Scattering
- Realtime Volumetric Cloud ( Based on Horizon Zero Dawn )
- Post-process effects (Tone-Mapping.)
- Event system
- Input (Keyboard, Mouse)
- Profiling (CPU & GPU)
- Batch2D Renderer
- Realtime Physics(Bullet3)

## Roadmap

Feature     					 	| Completion 	| Notes 
:-          					 	| :-         	| :-
Reflective Shadow Map				| 100%		  	| High priority
Light propagation volumes		 	| 90%       	    | High priority (No cascade)
Dynamic Diffus Global Illumination (DDGI)     | 90%             | High priority 
Ray-traced shadows				 	| 90%          	| Spatiotemporal Variance-Guided Filtering
Vulkan porting 	 				    | 90%	  		| support Compute and Tessellation shader
C# scripting                     	| 80%			| Using Mono (no engine API exposed yet)
Voxel Global Illumination     | 70%             | High priority 
Skinned Mesh                | 70%       | High priority
Precomputed Light Field Probes 	| 0%		  	| High priority
Screen space global illumination 	| 0%		  	| High priority
Animation and State Machine       	| 30%			| Apply Animation successfully
Precomputed Atmospheric Scattering 	| -          	| Low priority
Spherical Harmonics Lighting | -          | Medium priority
Subsurface Scattering 			| -          	| Low priority
Screen-space blurred reflections using Screen-Space Roughness | -          	| -
Ray-traced reflections			| -          	| -
SDF Shadow			| -          	| -
SDF AO			| -          	| -
GPU Driven Rendering			 	| -          	| High priority(https://vkguide.dev/docs/gpudriven/compute_culling/)
Culling System      			 	| -          	| High priority
Linux support			 	        | -          	| -
Android support			 	      | -          	| -
Mac support 			 	        | -          	| -
iOS support 			 	        | -          	| -
