# Advanced Graphics Programming
 
**Authors:** Arnau Falgueras García de Atocha and Marc Pagès Francesch

GitHub repo: https://github.com/marcpages2020/Advanced-Graphics-Programming

## Implemented features: 
  - Environment Mapping (Skybox, fake reflections and diffuse IBL)
  - PBR + PBR IBL

### Forward Rendering
|  Technique | No PBR  | PBR |
| ---------- | ------- | --- |
|No Environment Mapping| ![NoEnv_NoPBR](https://github.com/marcpages2020/Advanced-Graphics-Programming/blob/main/Captures/FRWD_NO_ENVMP_NO_PBR.PNG)|  ![NoEnv_PBR](https://github.com/marcpages2020/Advanced-Graphics-Programming/blob/main/Captures/FRWD_NO_ENVMP_PBR.PNG)|
|Environment Mapping| ![Env_NoPBR](https://github.com/marcpages2020/Advanced-Graphics-Programming/blob/main/Captures/FRWD_ENVMP_NO_PBR.PNG)| ![Env_PBR](https://github.com/marcpages2020/Advanced-Graphics-Programming/blob/main/Captures/FRWD_ENVMP_PBR.PNG)|

### Deferred Rendering
|  Technique | No PBR  | PBR |
| ---------- | ------- | --- |
|No Environment Mapping| ![NoEnv_NoPBR](https://github.com/marcpages2020/Advanced-Graphics-Programming/blob/main/Captures/DFRD_NO_ENVMP_NO_PBR.PNG)|  ![NoEnv_PBR](https://github.com/marcpages2020/Advanced-Graphics-Programming/blob/main/Captures/DFRD_NO_ENVMP_PBR.PNG)|
|Environment Mapping| ![Env_NoPBR](https://github.com/marcpages2020/Advanced-Graphics-Programming/blob/main/Captures/DFRD_ENVMP_NO_PBR.PNG)| ![Env_PBR](https://github.com/marcpages2020/Advanced-Graphics-Programming/blob/main/Captures/DFRD_ENVMP_PBR.PNG)|

## Engine Usage
To enable and disable the implemented techniques mark the checkboxes for each technique. To change between forward and deferred rendering use the dropdown under the checkboxes and to change between render targets use the dropdown under it. Under these you will find the foldables which let you change the entities positions, their metallic and roughness attributes. You will also find a foldable which lets you change the color of the lights, their positions and their direction. 

## Shader files
Under the Engine/WorkingDir/Shaders path you will find all the shaders used in the engine. For the skybox the used shader is the cubemap.glsl. For the forward rendering mode the used shaders are the forward_geometry.glsl to render the models without pbr, the pbr_forward_geometry.glsl to render the models with PBR and finally the forward_quad.glsl for the quad. For the deferred rendering mode the used shaders are the deferred_geometry.glsl for the models and deferred_quad.glsl to render without pbr and pbr_forward_geometry.glsl to render with pbr. To generate the support textures for pbr we are also using the brdf.glsl, the irradiance_map.glsl and the prefilter_map.glsl shaders. These generates the needed textures at the engine start to reflect the environment properly and handle the lights reflections. 
