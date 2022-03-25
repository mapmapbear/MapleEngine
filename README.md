
# Introduction 

Maple-Engine is my own engine for leaning mordern engine and rendering technique. 
                                                                                                                                                                        
![](https://flwmxd.github.io/images/MapleEngine.png)

### Realtime Volumetric Cloud 

![](https://flwmxd.github.io/images/cloud.png)

### Atmospheric Scattering
![](https://flwmxd.github.io/images/Atmosphere.png)

### PBR
![image](https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/PBR.png)

### Light Propagation Volumes

![image](https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/LPV_GI.png)

## Current Features

- OpenGL/Vulkan backends 
- Entity-component system( Based on entt )
- PBR/IBL
- Global Illumination(Light Propagation Volumes)
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


## Roadmap

Feature     					 	| Completion 	| Notes 
:-          					 	| :-         	| :-
Reflective Shadow Map				| 100%		  	| High priority
Light propagation volumes		 	| 100%       	    | High priority
C# scripting                     	| 80%			| Using Mono (no engine API exposed yet)
Vulkan porting 	 				    | 90%	  		| support Compute and Tessellation shader
Skinned Mesh                | 70%       | High priority
Precomputed Light Field Probes 	| 0%		  	| High priority
Screen space global illumination 	| 0%		  	| High priority
Animation and State Machine       	| 0%			| Implemented in Raven Engine
Precomputed Atmospheric Scattering 	| -          	| Low priority
Subsurface Scattering 			 	| -          	| Low priority
Ray traced shadows				 	| -          	| -
Ray traced reflections			 	| -          	| -
Linux support			 	        | -          	| -
Android support			 	        | -          	| -
Mac support 			 	        | -          	| -
iOS support 			 	        | -          	| -
