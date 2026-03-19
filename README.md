# MeshExtractor64

A DLL tool for extracting mesh, material and texture data from CryEngine 2 Sandbox Editor. Works with both native Crysis levels and Far Cry 1 (CryEngine 1) levels opened in CE2 Editor.

## What it does

Injects into `Editor.exe` and extracts all loaded static meshes with their materials and texture slots.

**Output files:**
- `C:\Users\Public\mesh_static.txt` — static meshes (buildings, props, architecture)
- `C:\Users\Public\mesh_vegetation.txt` — vegetation (trees, bushes, plants)

**Output format:**
```
Mesh: Objects/glm/ww2_indust_set1/WALLS/wall_B_200x400z.cgf
  Material_0: normalwallB
    Diffuse: Textures\ww2style\concrOld_893.dds
    Bump: Textures\ww2style\oldConcrete015C_ddn.dds
  Material_1: nodraw
    Diffuse: TEXTURES\Common\nodraw.dds
```

## How it works

- Scans `Cry3DEngine.dll` memory to find `gEnv` (global CryEngine environment)
- Calls `I3DEngine::GetLoadedStatObjArray()` to get all loaded meshes
- Reads materials and texture slots via `IMaterial` and `IRenderShaderResources`
- Filters duplicates — each unique mesh is written only once

## Requirements

- Crysis (2007) with Sandbox Editor
- Visual Studio 2022 (x64)
- Crysis Mod SDK 1.2 (for CryEngine headers) https://drive.google.com/file/d/154ng19l_ahaGxgspcesiyyDENj21Rqg4/view?usp=sharing
- DLL injector (e.g. Extreme Injector v3.7)

## Build

1. Open project in **Visual Studio 2022**
2. Set platform to **x64**
3. Add to `Additional Include Directories`:
   ```
   C:\CrysisSDK\Code\CryEngine\CryCommon
   C:\CrysisSDK\Code\CryEngine\CryAction
   ```
4. Build (`Ctrl+Shift+B`)

## Usage

1. Launch **Sandbox Editor**
2. Load any level
3. Run **Extreme Injector**
4. Select process: `Editor.exe`
5. Add `MeshExtractor64.dll`
6. Click **Inject**
7. Wait ~5 seconds
8. Check output files in `C:\Users\Public\`

## Notes

- Works with both Crysis and Far Cry 1 levels opened in CE2 Editor
- Extracts all meshes loaded in memory — not just visible ones
- No duplicates — each unique mesh path appears only once
- Filters out Editor helper objects and default meshes
