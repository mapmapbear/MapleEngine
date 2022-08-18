
# Introduction 

### ***Maple-Engine is my own engine for leaning mordern engine and rendering technique.***
                                                                                                                                                                       
                                                                                                                                                                       
![](https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/MapleEngine.png)

### Path Tracing (Hardware)

![image](https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/PathTrace.png)

### Voxel Cone Tracing

![image](https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/VXGI.png)

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
- Global Illumination(Light Propagation Volumes/Voxel Cone Tracing)
- Directional lights + Cascaded shadow maps
- Soft shadows (PCF)
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
Voxel Global Illumintaion     | 70%             | High priority 
Dynamic Diffus Global Illumintaion (DDGI)     | 0%             | High priority 
C# scripting                     	| 80%			| Using Mono (no engine API exposed yet)
Vulkan porting 	 				    | 90%	  		| support Compute and Tessellation shader
Skinned Mesh                | 70%       | High priority
Precomputed Light Field Probes 	| 0%		  	| High priority
Screen space global illumination 	| 0%		  	| High priority
Animation and State Machine       	| 30%			| Apply Animation successfully
Precomputed Atmospheric Scattering 	| -          	| Low priority
Spherical Harmonics Lighting | -          | Medium priority
Subsurface Scattering 			| -          	| Low priority
Screen-space blurred reflections using Screen-Space Roughness | -          	| -
Ray traced shadows				 	| 90%          	| Spatiotemporal Variance-Guided Filtering
Ray traced reflections			| -          	| -
GPU Driven Rendering			 	| -          	| High priority(https://vkguide.dev/docs/gpudriven/compute_culling/)
Culling System      			 	| -          	| High priority
Linux support			 	        | -          	| -
Android support			 	      | -          	| -
Mac support 			 	        | -          	| -
iOS support 			 	        | -          	| -
