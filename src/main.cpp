// GD Boost - system-level performance tuning for Geometry Dash (Windows).
// Nothing here touches rendering, particles or any visuals.

#include <Geode/Geode.hpp>

#ifdef GEODE_IS_WINDOWS
#include <Windows.h>
#include <string>

using namespace geode::prelude;

namespace {

    // Minimal definitions so the mod does not depend on the Windows SDK version.
    struct PowerThrottlingState {
        ULONG Version;
        ULONG ControlMask;
        ULONG StateMask;
    };
    constexpr ULONG kThrottlingVersion = 1;
    constexpr ULONG kThrottleExecutionSpeed = 0x1;
    constexpr ULONG kThrottleIgnoreTimerResolution = 0x4;
    constexpr int kProcessPowerThrottling = 4; // PROCESS_INFORMATION_CLASS::ProcessPowerThrottling

    template <class Fn>
    Fn loadFn(wchar_t const* dll, char const* name) {
        HMODULE module = LoadLibraryW(dll);
        if (!module) return nullptr;
        return reinterpret_cast<Fn>(GetProcAddress(module, name));
    }

    // Tell Windows to run this exe on the high-performance GPU.
    // Same thing as Settings > System > Display > Graphics > GeometryDash.exe > High performance.
    // Windows reads it when the process starts, so it applies from the next launch.
    void applyGpuPreference() {
        wchar_t exePath[MAX_PATH] = {};
        DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            log::warn("GPU preference: could not get the exe path");
            return;
        }

        HKEY key = nullptr;
        LSTATUS status = RegCreateKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\DirectX\\UserGpuPreferences",
            0, nullptr, REG_OPTION_NON_VOLATILE,
            KEY_QUERY_VALUE | KEY_SET_VALUE, nullptr, &key, nullptr
        );
        if (status != ERROR_SUCCESS) {
            log::warn("GPU preference: could not open registry key (error {})", static_cast<long>(status));
            return;
        }

        // Read the current value so other flags Windows stored there are preserved.
        std::wstring current;
        DWORD type = 0, size = 0;
        if (RegQueryValueExW(key, exePath, nullptr, &type, nullptr, &size) == ERROR_SUCCESS
            && type == REG_SZ && size >= sizeof(wchar_t)) {
            std::wstring buf(size / sizeof(wchar_t), L'\0');
            if (RegQueryValueExW(key, exePath, nullptr, &type, reinterpret_cast<LPBYTE>(buf.data()), &size) == ERROR_SUCCESS) {
                auto nul = buf.find(L'\0');
                current = (nul == std::wstring::npos) ? buf : buf.substr(0, nul);
            }
        }

        // GpuPreference: 0 = let Windows decide, 1 = power saving, 2 = high performance
        std::wstring result;
        bool found = false;
        size_t pos = 0;
        while (pos < current.size()) {
            size_t end = current.find(L';', pos);
            if (end == std::wstring::npos) end = current.size();
            std::wstring token = current.substr(pos, end - pos);
            pos = end + 1;
            if (token.empty()) continue;
            if (token.rfind(L"GpuPreference=", 0) == 0) {
                token = L"GpuPreference=2";
                found = true;
            }
            result += token + L";";
        }
        if (!found) result += L"GpuPreference=2;";

        if (result == current) {
            log::info("GPU preference: already set to high performance");
        }
        else {
            status = RegSetValueExW(
                key, exePath, 0, REG_SZ,
                reinterpret_cast<BYTE const*>(result.c_str()),
                static_cast<DWORD>((result.size() + 1) * sizeof(wchar_t))
            );
            if (status == ERROR_SUCCESS) {
                log::info("GPU preference: set to high performance (applies from the next launch)");
            }
            else {
                log::warn("GPU preference: could not write registry value (error {})", static_cast<long>(status));
            }
        }
        RegCloseKey(key);
    }

    void applyProcessPriority(std::string const& mode) {
        DWORD cls;
        if (mode == "Above normal") cls = ABOVE_NORMAL_PRIORITY_CLASS;
        else if (mode == "High") cls = HIGH_PRIORITY_CLASS;
        else return; // "Normal": leave the process alone

        if (SetPriorityClass(GetCurrentProcess(), cls)) {
            log::info("Process priority set to '{}'", mode);
        }
        else {
            log::warn("Could not set process priority (error {})", GetLastError());
        }
    }

    // Opt out of Efficiency Mode / EcoQoS so Windows does not park the game on slow cores
    // or lower its clocks, and stop it from ignoring our timer resolution request.
    void disablePowerThrottling() {
        using Fn = BOOL(WINAPI*)(HANDLE, int, LPVOID, DWORD);
        static Fn setProcessInformation = loadFn<Fn>(L"kernel32.dll", "SetProcessInformation");
        if (!setProcessInformation) {
            log::info("Power throttling: not supported on this Windows version");
            return;
        }

        PowerThrottlingState state{
            kThrottlingVersion,
            kThrottleExecutionSpeed | kThrottleIgnoreTimerResolution,
            0 // 0 in StateMask = throttling disabled for the controlled bits
        };
        if (setProcessInformation(GetCurrentProcess(), kProcessPowerThrottling, &state, sizeof(state))) {
            log::info("Power throttling disabled");
            return;
        }

        // Older Windows 10 builds do not know the timer-resolution bit: retry without it.
        state.ControlMask = kThrottleExecutionSpeed;
        if (setProcessInformation(GetCurrentProcess(), kProcessPowerThrottling, &state, sizeof(state))) {
            log::info("Power throttling disabled (execution speed only)");
        }
        else {
            log::warn("Could not disable power throttling (error {})", GetLastError());
        }
    }

    void raiseTimerResolution() {
        using Fn = UINT(WINAPI*)(UINT);
        static Fn timeBeginPeriod = loadFn<Fn>(L"winmm.dll", "timeBeginPeriod");
        if (timeBeginPeriod && timeBeginPeriod(1) == 0) {
            log::info("Timer resolution set to 1 ms");
        }
        else {
            log::warn("Could not set 1 ms timer resolution");
        }
    }

    // Must run on the game's main thread.
    void boostMainThread() {
        if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL)) {
            log::info("Main thread priority raised");
        }
        else {
            log::warn("Could not raise main thread priority (error {})", GetLastError());
        }

        // MMCSS "Games" profile: the same scheduling class games use for their render thread.
        using Fn = HANDLE(WINAPI*)(LPCWSTR, LPDWORD);
        static Fn setMmThreadCharacteristics = loadFn<Fn>(L"avrt.dll", "AvSetMmThreadCharacteristicsW");
        if (setMmThreadCharacteristics) {
            DWORD taskIndex = 0;
            if (setMmThreadCharacteristics(L"Games", &taskIndex)) {
                log::info("Main thread registered with MMCSS 'Games' profile");
            }
            else {
                log::warn("MMCSS registration failed (error {})", GetLastError());
            }
        }
    }

} // namespace

$on_mod(Loaded) {
    auto mod = Mod::get();

    if (mod->getSettingValue<bool>("gpu-preference")) applyGpuPreference();
    applyProcessPriority(mod->getSettingValue<std::string>("process-priority"));
    if (mod->getSettingValue<bool>("disable-throttling")) disablePowerThrottling();
    if (mod->getSettingValue<bool>("timer-resolution")) raiseTimerResolution();
    if (mod->getSettingValue<bool>("main-thread-boost")) {
        queueInMainThread([] { boostMainThread(); });
    }
}

#else

using namespace geode::prelude;

$on_mod(Loaded) {
    log::info("GD Boost only does something on Windows");
}

#endif
