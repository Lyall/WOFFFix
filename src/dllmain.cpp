#include "stdafx.h"
#include "helper.hpp"
#include <inipp/inipp.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <safetyhook.hpp>

HMODULE baseModule = GetModuleHandle(NULL);
HMODULE thisModule;

// Logger and config setup
inipp::Ini<char> ini;
std::shared_ptr<spdlog::logger> logger;
string sFixName = "WOFFFix";
string sFixVer = "0.8.0";
string sLogFile = "WOFFFix.log";
string sConfigFile = "WOFFFix.ini";
string sWindowClassName = "SiliconStudio Inc.";
string sExeName;
filesystem::path sExePath;
filesystem::path sThisModulePath;
std::pair DesktopDimensions = { 0,0 };

// Ini Variables
bool bCustomResolution;
int iCustomResX;
int iCustomResY;
bool bWindowedMode;
bool bBorderlessMode;
bool bAspectFix;
bool bFOVFix;
bool bHUDFix;
bool bHideCursor;
bool bUncapFPS;
bool bShadowRes;
int iShadowRes;

// Aspect ratio + HUD stuff
float fPi = (float)3.141592653;
float fNativeAspect = (float)16 / 9;
float fNativeWidth;
float fNativeHeight;
float fAspectRatio;
float fAspectMultiplier;
float fHUDWidth;
float fHUDHeight;
float fDefaultHUDWidth = (float)1920;
float fDefaultHUDHeight = (float)1080;
float fHUDWidthOffset;
float fHUDHeightOffset;

// Variables
int iResX;
int iResY;
float fCurrentFrametime;
int iCreateWindowCount;

// CreateWindowExW Hook
SafetyHookInline CreateWindowExW_hook{};
HWND WINAPI CreateWindowExW_hooked(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    auto hWnd = CreateWindowExW_hook.stdcall<HWND>(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    // This is jank, probably better to compare class name?
    iCreateWindowCount++;
    if (iCreateWindowCount < 2)
    {
        LONG lStyle = GetWindowLong(hWnd, GWL_STYLE);
        if ((lStyle & WS_POPUP) != WS_POPUP)
        {
            lStyle &= ~(WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
            SetWindowLong(hWnd, GWL_STYLE, lStyle);
        }

        // Maximize window and put window on top
        SetWindowPos(hWnd, HWND_TOP, 0, 0, DesktopDimensions.first, DesktopDimensions.second, NULL);

        spdlog::info("CreateWindowExW_hook: Set borderless mode and maximized window.");
    }

    // Set window focus
    SetFocus(hWnd);

    return hWnd;
}

HCURSOR hCursor;
SafetyHookInline LoadCursorW_hook{};
HCURSOR WINAPI LoadCursorW_hooked(HINSTANCE hInstance, LPCWSTR name)
{
    // Disable mouse cursor
    spdlog::info("LoadCursorW_hook: Disabled ShowCursor.");
    ShowCursor(false);
    return LoadCursorW_hook.stdcall<HCURSOR>(hInstance, name);
}

void Logging()
{
    // Get this module path
    WCHAR thisModulePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(thisModule, thisModulePath, MAX_PATH);
    sThisModulePath = thisModulePath;
    sThisModulePath = sThisModulePath.remove_filename();

    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

    // spdlog initialisation
    {
        try
        {
            logger = spdlog::basic_logger_st(sFixName.c_str(), sThisModulePath.string() + sLogFile, true);
            spdlog::set_default_logger(logger);

            spdlog::flush_on(spdlog::level::debug);
            spdlog::info("----------");
            spdlog::info("{} v{} loaded.", sFixName.c_str(), sFixVer.c_str());
            spdlog::info("----------");
            spdlog::info("Path to logfile: {}", sThisModulePath.string() + sLogFile);
            spdlog::info("----------");

            // Log module details
            spdlog::info("Module Name: {0:s}", sExeName.c_str());
            spdlog::info("Module Path: {0:s}", sExePath.string());
            spdlog::info("Module Address: 0x{0:x}", (uintptr_t)baseModule);
            spdlog::info("Module Timestamp: {0:d}", Memory::ModuleTimestamp(baseModule));
            spdlog::info("----------");
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            AllocConsole();
            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            std::cout << "Log initialisation failed: " << ex.what() << std::endl;
        }
    }
}

void ReadConfig()
{
    // Initialise config
    std::ifstream iniFile(sThisModulePath.string() + sConfigFile);
    if (!iniFile)
    {
        AllocConsole();
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "" << sFixName.c_str() << " v" << sFixVer.c_str() << " loaded." << std::endl;
        std::cout << "ERROR: Could not locate config file." << std::endl;
        std::cout << "ERROR: Make sure " << sConfigFile.c_str() << " is located in " << sThisModulePath.string().c_str() << std::endl;
    }
    else
    {
        spdlog::info("Path to config file: {}", sThisModulePath.string() + sConfigFile);
        ini.parse(iniFile);
    }

    // Read ini file
    inipp::get_value(ini.sections["Custom Resolution"], "Enabled", bCustomResolution);
    inipp::get_value(ini.sections["Custom Resolution"], "Width", iCustomResX);
    inipp::get_value(ini.sections["Custom Resolution"], "Height", iCustomResY);
    inipp::get_value(ini.sections["Custom Resolution"], "Windowed", bWindowedMode);
    inipp::get_value(ini.sections["Custom Resolution"], "Borderless", bBorderlessMode);
    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bAspectFix);
    inipp::get_value(ini.sections["Fix FOV"], "Enabled", bFOVFix);
    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bHUDFix);
    inipp::get_value(ini.sections["Hide Mouse Cursor"], "Enabled", bHideCursor);
    inipp::get_value(ini.sections["Unlock Framerate"], "Enabled", bUncapFPS);
    inipp::get_value(ini.sections["Shadow Resolution"], "Enabled", bShadowRes);
    inipp::get_value(ini.sections["Shadow Resolution"], "Resolution", iShadowRes);

    // Log config parse
    spdlog::info("Config Parse: bCustomResolution: {}", bCustomResolution);
    spdlog::info("Config Parse: iCustomResX: {}", iCustomResX);
    spdlog::info("Config Parse: iCustomResY: {}", iCustomResY);
    spdlog::info("Config Parse: bWindowedMode: {}", bWindowedMode);
    spdlog::info("Config Parse: bBorderlessMode: {}", bBorderlessMode);
    if (bBorderlessMode && !bWindowedMode)
    {
        spdlog::info("Config Parse: bBorderlessMode enabled. Enabling bWindowedMode.");
        bWindowedMode = true;
    }
    spdlog::info("Config Parse: bAspectFix: {}", bAspectFix);
    spdlog::info("Config Parse: bFOVFix: {}", bFOVFix);
    spdlog::info("Config Parse: bHUDFix: {}", bHUDFix);
    spdlog::info("Config Parse: bHideCursor: {}", bHideCursor);
    spdlog::info("Config Parse: bUncapFPS: {}", bUncapFPS);
    spdlog::info("Config Parse: bShadowRes: {}", bShadowRes);
    spdlog::info("Config Parse: iShadowRes: {}", iShadowRes);
    spdlog::info("----------");

    // Get desktop resolution
    DesktopDimensions = Util::GetPhysicalDesktopDimensions();

    // Set custom resolution to desktop resolution
    if (iCustomResX == 0 || iCustomResY == 0)
    {
        iCustomResX = (int)DesktopDimensions.first;
        iCustomResY = (int)DesktopDimensions.second;
        spdlog::info("Custom Resolution: Desktop Width: {}", iCustomResX);
        spdlog::info("Custom Resolution: Desktop Height: {}", iCustomResY);
    }
}

void Resolution()
{
    // Apply custom resolution
    uint8_t* ApplyResolutionScanResult = Memory::PatternScan(baseModule, "89 ?? ?? 89 ?? ?? E9 ?? ?? ?? ?? 48 8D ?? ?? ?? ?? ?? 49 ?? ?? E8 ?? ?? ?? ?? 85 ?? 75 ?? 48 ?? ?? 02");
    if (ApplyResolutionScanResult)
    {
        spdlog::info("Custom Resolution: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)ApplyResolutionScanResult - (uintptr_t)baseModule);

        static SafetyHookMid ApplyResolutionMidHook{};
        ApplyResolutionMidHook = safetyhook::create_mid(ApplyResolutionScanResult,
            [](SafetyHookContext& ctx)
            {
                if (bCustomResolution)
                {
                    ctx.rbx = iCustomResX;
                    ctx.rax = iCustomResY;
                }

                if (ctx.rsi + 0xC)
                {
                    // Set windowed/fullscreen
                    *reinterpret_cast<int*>(ctx.rsi + 0xC) = (int)!bWindowedMode;
                }

                iResX = (int)ctx.rbx;
                iResY = (int)ctx.rax;

                fAspectRatio = (float)iResX / iResY;
                fAspectMultiplier = fAspectRatio / fNativeAspect;
                fNativeWidth = (float)iResY * fNativeAspect;
                fNativeHeight = (float)iResX / fNativeAspect;

                // HUD variables
                fHUDWidth = (float)iResY * fNativeAspect;
                fHUDHeight = (float)iResY;
                fHUDWidthOffset = (float)(iResX - fHUDWidth) / 2;
                fHUDHeightOffset = 0;
                if (fAspectRatio < fNativeAspect)
                {
                    fHUDWidth = (float)iResX;
                    fHUDHeight = (float)iResX / fNativeAspect;
                    fHUDWidthOffset = 0;
                    fHUDHeightOffset = (float)(iResY - fHUDHeight) / 2;
                }

                // Log aspect ratio stuff
                spdlog::info("----------");
                spdlog::info("Resolution: Resolution: {}x{}", iResX, iResY);
                spdlog::info("Resolution: fAspectRatio: {}", fAspectRatio);
                spdlog::info("Resolution: fAspectMultiplier: {}", fAspectMultiplier);
                spdlog::info("Resolution: fNativeWidth: {}", fNativeWidth);
                spdlog::info("Resolution: fNativeHeight: {}", fNativeHeight);
                spdlog::info("Resolution: fHUDWidth: {}", fHUDWidth);
                spdlog::info("Resolution: fHUDHeight: {}", fHUDHeight);
                spdlog::info("Resolution: fHUDWidthOffset: {}", fHUDWidthOffset);
                spdlog::info("Resolution: fHUDHeightOffset: {}", fHUDHeightOffset);
                spdlog::info("----------");
            });
    }
    else if (!ApplyResolutionScanResult)
    {
        spdlog::error("Custom Resolution: Pattern scan failed.");
    }

    if (bBorderlessMode)
    {
        // Hook CreateWindowExW so we can apply borderless style and maximize
        CreateWindowExW_hook = safetyhook::create_inline(&CreateWindowExW, reinterpret_cast<void*>(CreateWindowExW_hooked));
    }

    if (bHideCursor)
    {
        // Hook LoadCursorW to hide mouse cursor
        LoadCursorW_hook = safetyhook::create_inline(&LoadCursorW, reinterpret_cast<void*>(LoadCursorW_hooked));
    }
}

void AspectFOV()
{
    if (bAspectFix)
    {
        // Aspect Ratio
        uint8_t* AspectRatioScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? 00 F3 0F ?? ?? ?? ?? 0F 28 ?? 48 8B ?? ?? ?? ?? ?? 00");
        if (AspectRatioScanResult)
        {
            spdlog::info("Aspect Ratio: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)AspectRatioScanResult - (uintptr_t)baseModule);

            static SafetyHookMid AspectRatioMidHook{};
            AspectRatioMidHook = safetyhook::create_mid(AspectRatioScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (ctx.rax + 0x280)
                    {
                        *reinterpret_cast<float*>(ctx.rax + 0x280) = fAspectRatio;
                    }
                });
        }
        else if (!AspectRatioScanResult)
        {
            spdlog::error("Aspect Ratio: Pattern scan failed.");
        }
    }

    if (bFOVFix)
    {
        // FOV
        uint8_t* GameplayFOVScanResult = Memory::PatternScan(baseModule, "F3 ?? ?? ?? ?? ?? ?? ?? ?? 0F ?? ?? ?? ?? ?? ?? 0F ?? ?? ?? ?? ?? ?? 76 ?? 0F ?? ?? 0F ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 73 ??");
        uint8_t* CutsceneFOVScanResult = Memory::PatternScan(baseModule, "89 ?? ?? ?? ?? ?? F3 ?? ?? ?? ?? 41 ?? ?? ?? F3 ?? ?? ?? ?? F3 0F ?? ?? 0F ?? ??");
        if (GameplayFOVScanResult && CutsceneFOVScanResult)
        {
            spdlog::info("Gameplay FOV: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GameplayFOVScanResult - (uintptr_t)baseModule);
            static SafetyHookMid GameplayFOVMidHook{};
            GameplayFOVMidHook = safetyhook::create_mid(GameplayFOVScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio < fNativeAspect)
                    {
                        ctx.xmm8.f32[0] = atanf(tanf(ctx.xmm8.f32[0] * (fPi / 360)) / fAspectRatio * fNativeAspect) * (360 / fPi);
                    }
                }); 

            spdlog::info("Cutscene FOV: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)CutsceneFOVScanResult - (uintptr_t)baseModule);
            static SafetyHookMid CutsceneFOVMidHook{};
            CutsceneFOVMidHook = safetyhook::create_mid(CutsceneFOVScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio < fNativeAspect)
                    {
                        float fov = *reinterpret_cast<float*>(&ctx.rax);
                        float newFov = atanf(tanf(fov * (fPi / 360)) / fAspectRatio * fNativeAspect) * (360 / fPi);
                        ctx.rax = *(uint32_t*)&newFov;
                    }
                });
        }
        else if (!GameplayFOVScanResult || !CutsceneFOVScanResult)
        {
            spdlog::error("FOV: Pattern scan failed.");
        }
    }
}

void HUD()
{
    if (bHUDFix)
    {
        uint8_t* HUDScanResult = Memory::PatternScan(baseModule, "41 ?? ?? ?? 0F ?? ?? F3 0F ?? ?? ?? ?? E8 ?? ?? ?? ??") + 0x4;
        if (HUDScanResult)
        {
            spdlog::info("HUD: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)HUDScanResult - (uintptr_t)baseModule);

            static SafetyHookMid HUDMidHook{};
            HUDMidHook = safetyhook::create_mid(HUDScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio > fNativeAspect)
                    {
                        float HUDWidth = ceilf((float)544 * fAspectRatio);
                        float HUDWidthOffset = ceilf((HUDWidth - 960.00f) / 2.00f);
                        ctx.xmm2.u32[0] = (int)ceilf(HUDWidth - HUDWidthOffset);
                        ctx.xmm1.f32[0] = -HUDWidthOffset;
                    }
                    else if (fAspectRatio < fNativeAspect)
                    {
                        float HUDHeight = ceilf((float)960 / fAspectRatio);
                        float HUDHeightOffset = ceilf((HUDHeight - 544.00f) / 2.00f);
                        ctx.xmm0.f32[0] = ceilf(HUDHeight - HUDHeightOffset);
                        ctx.xmm3.f32[0] = -HUDHeightOffset;
                    }
                });
        }
        else if (!HUDScanResult)
        {
            spdlog::error("HUD: Pattern scan failed.");
        }
    }  
}

void Framerate()
{
    if (bUncapFPS)
    {
        uint8_t* GameSpeed1ScanResult = Memory::PatternScan(baseModule, "EB ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? F3 0F ?? ?? ?? ?? 48 ?? ?? ?? C3");
        uint8_t* FPSCapScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? F3 0F ?? ?? ?? ?? F3 0F ?? ?? ?? ?? 48 ?? ?? ?? ?? ?? ?? 00 83 ?? ?? ?? ?? ?? 00 74 ??");
        uint8_t* GameSpeed2ScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? F3 0F ?? ?? ?? ?? F3 0F ?? ?? 0F 28 ?? F3 0F ?? ?? ?? ?? 48 ?? ?? ?? ?? 8B ?? ?? 83 ?? 10");
        uint8_t* CurrentFrametimeScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? 48 ?? ?? ?? ?? 48 ?? ?? ?? ?? ?? 48 ?? ?? E8 ?? ?? ?? ?? 48 ?? ?? ?? ?? 83 ?? ?? ?? ?? ?? 00 74 ??");
        if (GameSpeed1ScanResult && FPSCapScanResult && GameSpeed2ScanResult && CurrentFrametimeScanResult)
        {
            // Set FPS cap to 0
            spdlog::info("Unlock Framerate: FPS Cap: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)FPSCapScanResult - (uintptr_t)baseModule);
            static SafetyHookMid FPSCapMidHook{};
            FPSCapMidHook = safetyhook::create_mid(FPSCapScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (ctx.rsp + 0x3C)
                    {
                        *reinterpret_cast<float*>(ctx.rsp + 0x3C) = 0.0f; // Hopefully setting it to 0 doesn't cause problems ;)
                    }
                });

            // Grab current frametime
            spdlog::info("Unlock Framerate: Current Frametime: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)CurrentFrametimeScanResult - (uintptr_t)baseModule);
            static SafetyHookMid CurrentFrametimeMidHook{};
            CurrentFrametimeMidHook = safetyhook::create_mid(CurrentFrametimeScanResult,
                [](SafetyHookContext& ctx)
                {
                    fCurrentFrametime = ctx.xmm0.f32[0];
                });

            // Game speed (3D stuff)
            spdlog::info("Unlock Framerate: Game Speed 1: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GameSpeed1ScanResult - (uintptr_t)baseModule);
            static SafetyHookMid GameSpeed1MidHook{};
            GameSpeed1MidHook = safetyhook::create_mid(GameSpeed1ScanResult + 0x16,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm0.f32[0] = 1000.0f / fCurrentFrametime;
                });

            // Game speed (animations?)
            spdlog::info("Unlock Framerate: Game Speed 2: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GameSpeed2ScanResult - (uintptr_t)baseModule);
            static SafetyHookMid GameSpeed2MidHook{};
            GameSpeed2MidHook = safetyhook::create_mid(GameSpeed2ScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm0.f32[0] = (1000.0f / fCurrentFrametime) / 30.0f;
                });
        }
        else if (!FPSCapScanResult || !GameSpeed1ScanResult || !GameSpeed2ScanResult || !CurrentFrametimeScanResult)
        {
            spdlog::error("Unlock Framerate: Pattern scan failed.");
        }
    }
}

void GraphicalTweaks()
{
    if (bShadowRes)
    {
        // Shadow Resolution
        uint8_t* ShadowResScanResult = Memory::PatternScan(baseModule, "89 ?? ?? 89 ?? ?? E9 ?? ?? ?? ?? 48 8D ?? ?? ?? ?? ?? 49 ?? ??");
        if (ShadowResScanResult)
        {
            spdlog::info("Shadow Resolution: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)ShadowResScanResult - (uintptr_t)baseModule);

            static SafetyHookMid ShadowResMidHook{};
            ShadowResMidHook = safetyhook::create_mid(ShadowResScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.rax = (int)8192;
                });
        }
        else if (!ShadowResScanResult)
        {
            spdlog::error("Shadow Resolution: Pattern scan failed.");
        }
    }
}

std::mutex mainThreadFinishedMutex;
std::condition_variable mainThreadFinishedVar;
bool mainThreadFinished = false;

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    Resolution();
    AspectFOV();
    HUD();
    Framerate();
    GraphicalTweaks();

    // Signal any threads which might be waiting for us before continuing
    {
        std::lock_guard lock(mainThreadFinishedMutex);
        mainThreadFinished = true;
        mainThreadFinishedVar.notify_all();
    }

    return true;
}

std::mutex memsetHookMutex;
bool memsetHookCalled = false;
void* (__cdecl* memset_Fn)(void* Dst, int Val, size_t Size);
void* __cdecl memset_Hook(void* Dst, int Val, size_t Size)
{
    // memset is one of the first imports called by game (not the very first though, since ASI loader still has those hooked during our DllMain...)
    std::lock_guard lock(memsetHookMutex);
    if (!memsetHookCalled)
    {
        memsetHookCalled = true;

        // First we'll unhook the IAT for this function as early as we can
        Memory::HookIAT(baseModule, "VCRUNTIME140.dll", memset_Hook, memset_Fn);

        // Wait for our main thread to finish before we return to the game
        if (!mainThreadFinished)
        {
            std::unique_lock finishedLock(mainThreadFinishedMutex);
            mainThreadFinishedVar.wait(finishedLock, [] { return mainThreadFinished; });
        }
    }

    return memset_Fn(Dst, Val, Size);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        // Try hooking IAT of one of the imports game calls early on, so we can make it wait for our Main thread to complete before returning back to game
        // This will only hook the main game modules usage of memset, other modules calling it won't be affected
        HMODULE vcruntime140 = GetModuleHandleA("VCRUNTIME140.dll");
        if (vcruntime140)
        {
            memset_Fn = decltype(memset_Fn)(GetProcAddress(vcruntime140, "memset"));
            Memory::HookIAT(baseModule, "VCRUNTIME140.dll", memset_Fn, memset_Hook);
        }

        thisModule = hModule;
        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);
        if (mainHandle)
        {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_HIGHEST); // set our Main thread priority higher than the games thread
            CloseHandle(mainHandle);
        }
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

