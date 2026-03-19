#include "pch.h"
#include <windows.h>
#include <psapi.h>
#include <stdio.h>

#pragma comment(lib, "psapi.lib")

struct IParticleEmitter;

#include "ISystem.h"
#include "I3DEngine.h"
#include "IStatObj.h"
#include "IMaterial.h"
#include "IShader.h"

bool CryAssert(const char*, const char*, unsigned int, bool*)
{
    return false;
}

void CryAssertTrace(const char*, ...)
{
}

void CryDebugBreak()
{
}

SSystemGlobalEnvironment* FindGEnv()
{
    HMODULE hCrySystem = GetModuleHandleA("CrySystem.dll");
    HMODULE hCry3D = GetModuleHandleA("Cry3DEngine.dll");
    if (!hCrySystem || !hCry3D) return NULL;

    MODULEINFO miSys = {};
    GetModuleInformation(GetCurrentProcess(), hCrySystem, &miSys, sizeof(miSys));
    BYTE* pSysBase = (BYTE*)hCrySystem;
    BYTE* pSysEnd = pSysBase + miSys.SizeOfImage;

    MODULEINFO mi3D = {};
    GetModuleInformation(GetCurrentProcess(), hCry3D, &mi3D, sizeof(mi3D));
    BYTE* p3DBase = (BYTE*)hCry3D;
    BYTE* p3DEnd = p3DBase + mi3D.SizeOfImage;

    for (BYTE* p = p3DBase; p < p3DEnd - 8; p += 8)
    {
        __try
        {
            SSystemGlobalEnvironment* pEnv = *(SSystemGlobalEnvironment**)p;
            if (!pEnv) continue;
            void** vtSys = *(void***)pEnv->pSystem;
            if (!vtSys) continue;
            if ((BYTE*)vtSys < pSysBase || (BYTE*)vtSys >= pSysEnd) continue;
            void** vt3D = *(void***)pEnv->p3DEngine;
            if (!vt3D) continue;
            if ((BYTE*)vt3D < p3DBase || (BYTE*)vt3D >= p3DEnd) continue;
            return pEnv;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {}
    }
    return NULL;
}

static char g_processed[8192][260];
static int g_processedCount = 0;

bool IsProcessed(const char* name)
{
    for (int i = 0; i < g_processedCount; i++)
        if (_stricmp(g_processed[i], name) == 0) return true;
    return false;
}

void AddProcessed(const char* name)
{
    if (g_processedCount < 8192)
        strncpy_s(g_processed[g_processedCount++], 260, name, 259);
}

void WriteMeshInfo(FILE* f, IStatObj* pStatObj)
{
    __try
    {
        const char* meshPath = pStatObj->GetFilePath();
        if (!meshPath || meshPath[0] == 0) return;

        fprintf(f, "\nMesh: %s\n", meshPath);

        IMaterial* pMat = pStatObj->GetMaterial();
        if (!pMat) return;

        int subCount = pMat->GetSubMtlCount();
        if (subCount == 0) subCount = 1;

        for (int m = 0; m < subCount; m++)
        {
            IMaterial* pSubMat = (subCount == 1) ? pMat : pMat->GetSubMtl(m);
            if (!pSubMat) continue;

            fprintf(f, "  Material_%d: %s\n", m, pSubMat->GetName());

            SShaderItem& si = pSubMat->GetShaderItem(0);
            if (!si.m_pShaderResources) continue;

            const char* slotNames[] = {
                "Diffuse","Bump","Gloss","Env","DetailOverlay",
                "BumpDiffuse","Phong","BumpHeight","Specular","DecalOverlay",
                "Normalmap","Subsurface","Custom","CustomSecondary","Opacity"
            };

            for (int t = 0; t < 15; t++)
            {
                SEfResTexture* pTex = si.m_pShaderResources->GetTexture(t);
                if (!pTex || pTex->m_Name.empty()) continue;
                fprintf(f, "    %s: %s\n", slotNames[t], pTex->m_Name.c_str());
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {}
}

DWORD WINAPI ExtractThread(LPVOID)
{
    Sleep(3000);

    SSystemGlobalEnvironment* gEnv = FindGEnv();
    if (!gEnv)
    {
        MessageBoxA(NULL, "gEnv not found!", "Error", MB_OK);
        return 0;
    }

    int nCount = 0;
    __try
    {
        gEnv->p3DEngine->GetLoadedStatObjArray(NULL, nCount);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        MessageBoxA(NULL, "Crash in GetLoadedStatObjArray!", "Error", MB_OK);
        return 0;
    }

    if (nCount <= 0)
    {
        MessageBoxA(NULL, "No StatObj found!", "Error", MB_OK);
        return 0;
    }

    IStatObj** pObjects = new IStatObj*[nCount];
    memset(pObjects, 0, sizeof(IStatObj*) * nCount);

    __try
    {
        gEnv->p3DEngine->GetLoadedStatObjArray(pObjects, nCount);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        delete[] pObjects;
        MessageBoxA(NULL, "Crash getting array!", "Error", MB_OK);
        return 0;
    }

    FILE* fStatic = fopen("C:\\Users\\Public\\mesh_static.txt", "w");
    FILE* fVeg    = fopen("C:\\Users\\Public\\mesh_vegetation.txt", "w");

    if (!fStatic || !fVeg)
    {
        delete[] pObjects;
        MessageBoxA(NULL, "Cannot create files!", "Error", MB_OK);
        return 0;
    }

    int staticCount = 0;
    int vegCount = 0;

    for (int i = 0; i < nCount; i++)
    {
        if (!pObjects[i]) continue;

        __try
        {
            const char* meshPath = pObjects[i]->GetFilePath();
            if (!meshPath || meshPath[0] == 0) continue;
            if (strstr(meshPath, "editor") != NULL) continue;
            if (strstr(meshPath, "Editor") != NULL) continue;
            if (strstr(meshPath, "default") != NULL) continue;
            if (IsProcessed(meshPath)) continue;
            AddProcessed(meshPath);

            if (strstr(meshPath, "Natural") != NULL ||
                strstr(meshPath, "natural") != NULL)
            {
                WriteMeshInfo(fVeg, pObjects[i]);
                vegCount++;
            }
            else
            {
                WriteMeshInfo(fStatic, pObjects[i]);
                staticCount++;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {}
    }

    delete[] pObjects;
    fclose(fStatic);
    fclose(fVeg);

    char msg[256];
    sprintf_s(msg,
        "Done!\nStatic meshes: %d\nVegetation: %d\n\n"
        "mesh_static.txt\n"
        "mesh_vegetation.txt",
        staticCount, vegCount);
    MessageBoxA(NULL, msg, "Result", MB_OK);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        CreateThread(NULL, 0, ExtractThread, NULL, 0, NULL);
    }
    return TRUE;
}
