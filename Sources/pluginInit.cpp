#include "3DS.h"
#include "CTRPluginFrameworkImpl.hpp"
#include "CTRPluginFramework.hpp"
#include "CTRPluginFrameworkImpl/Graphics/Font.hpp"
#include "CTRPluginFrameworkImpl/System/Screenshot.hpp"
#include "csvc.h"

extern "C"
{
    Thread  g_mainThread = nullptr; ///< Used in syscalls.c for __ctru_get_reent
    u32     __ctru_heap_size;
    u32     g_gspEventThreadPriority;

    void    LoadCROHooked(void);
    void    OnLoadCro(void);
    void    abort(void);
    void    initSystem();
    void    initLib();
    Result  __sync_init(void);
    void    __system_initSyscalls(void);
}

using CTRPluginFramework::Hook;

namespace CTRPluginFramework
{
    // Threads stacks
    static u8  threadStack[0x4000] ALIGN(8);
    static u8  keepThreadStack[0x1000] ALIGN(8);

    // Some globals
    FS_Archive  _sdmcArchive;
    Handle      g_continueGameEvent = 0;
    Handle      g_keepThreadHandle;
    Handle      g_keepEvent = 0;
    Handle      g_resumeEvent = 0;
    bool        g_keepRunning = true;
    __attribute__((weak)) u32         ___heap_size = 0;
    __attribute__((weak)) u32         ___eco_memory_mode = 0;

    extern bool     g_heapError; ///< allocateHeaps.cpp

    void    ThreadInit(void *arg);
    void    ThreadExit(void);
    void    InstallOSD(void); ///< OSDImpl
    void    InitializeRandomEngine(void);

    // From main.cpp
    void    PatchProcess(FwkSettings &settings);
    int     main(void);
}

namespace Kernel
{
    void    Initialize(void);
}

static Hook         g_onLoadCroHook;
static LightLock    g_onLoadCroLock;

void abort(void)
{
    if (CTRPluginFramework::System::OnAbort)
        CTRPluginFramework::System::OnAbort();

    CTRPluginFramework::Color c(255, 69, 0);
    CTRPluginFramework::ScreenImpl::Top->Flash(c);
    CTRPluginFramework::ScreenImpl::Bottom->Flash(c);

    CTRPluginFramework::ThreadExit();
    while (true);
}

static void    ExecuteLoopOnEvent(void)
{
    using CTRPluginFramework::PluginMenuExecuteLoop;

    // Execute AR codes
    PluginMenuExecuteLoop::LockAR();
    PluginMenuExecuteLoop::ExecuteAR();
    PluginMenuExecuteLoop::UnlockAR();

    // Execute builtin codes
    PluginMenuExecuteLoop::Lock();
    PluginMenuExecuteLoop::ExecuteBuiltin();
    PluginMenuExecuteLoop::Unlock();
}

void     OnLoadCro(void)
{
    LightLock_Lock(&g_onLoadCroLock);
    ExecuteLoopOnEvent();
    LightLock_Unlock(&g_onLoadCroLock);
}

namespace CTRPluginFramework
{
    namespace Heap
    {
        extern u8* __ctrpf_heap;
        extern u32 __ctrpf_heap_size;
    }
    static void InitFS(void)
    {
        // Init sdmcArchive
        {
            FS_Path sdmcPath = { PATH_EMPTY, 1, (u8*)"" };
            FSUSER_OpenArchive(&_sdmcArchive, ARCHIVE_SDMC, sdmcPath);
        }

        // Set current working directory
        if (!System::IsLoaderNTR()) // Luma's loader
        {
            if (*(vu32 *)0x070000FC)
            {
                std::string path = Utils::Format("/luma/plugins/ActionReplay/%016llX", Process::GetTitleID());

                if (!Directory::IsExists(path))
                    Directory::Create(path);

                Directory::ChangeWorkingDirectory(path + "/");
            }
            else
                Directory::ChangeWorkingDirectory(Utils::Format("/luma/plugins/%016llX/", Process::GetTitleID()));
        }
        else
        {
            std::string dirpath = Utils::Format("/plugin/%016llX", Process::GetTitleID());

            // Check if game's folder exists
            if (!Directory::IsExists(dirpath))
            {
                // If doesn't exist, so create a temporary folder
                dirpath = "/plugin/game/ctrpf";
                if (!Directory::IsExists(dirpath))
                    Directory::Create(dirpath);
                dirpath += "/";
                Process::GetName(dirpath);

                if (!Directory::IsExists(dirpath))
                    Directory::Create(dirpath);
            }

            dirpath += "/";
            Directory::ChangeWorkingDirectory(dirpath);
        }
    }

    static void     InitHeap(void)
    {
        u32         size;

        if (___eco_memory_mode)
            size = 0x50000;
        else if (System::IsLoaderNTR())
            size = 0xC0000;
        else if (!SystemImpl::IsNew3DS)
            size = 0x120000;
        else
            size = 0x100000;

        // Init System::Heap
        Heap::__ctrpf_heap_size = size;
        Heap::__ctrpf_heap = static_cast<u8 *>(::operator new(size));
    }

    void    KeepThreadMain(void *arg UNUSED)
    {
        // Initialize the synchronization subsystem
        __sync_init();

        // Initialize newlib's syscalls
        __system_initSyscalls();

        // Initialize services
        srvInit();
        acInit();
        amInit();
        fsInit();
        cfguInit();

        // Initialize Kernel stuff
        Kernel::Initialize();

        // Init Framework's system constants
        SystemImpl::Initialize();

        // Init Process info
        ProcessImpl::Initialize();

        // Init Screen
        ScreenImpl::Initialize();

        // Init locks
        PluginMenuExecuteLoop::InitLocks();

        // Protect code
        Process::CheckAddress(0x00100000, 7);
        Process::CheckAddress(0x00102000, 7); ///< NTR seems to protect the first 0x1000 bytes, which split the region in two
        // Protect VRAM
        Process::ProtectRegion(0x1F000000, 3);

        // Check loader
        SystemImpl::IsLoaderNTR = Process::CheckAddress(0x06000000, 3);

        // Define heap size
        __ctru_heap_size = SystemImpl::IsLoaderNTR ? 0x100000 : 0x200000;
        __ctru_heap_size = std::max(__ctru_heap_size, ___heap_size);

        // Patch KProcess::ResourceLimit
        ProcessImpl::KProcessPtr->PatchMaxCommit(__ctru_heap_size + 0x1000);
        if (*(u32 *)0x1FF80030 == 3)
            *(u32 *)(PA_FROM_VA(0x1FF80048)) += __ctru_heap_size + 0x1000;
        else
            *(u32 *)(PA_FROM_VA(0x1FF80040)) += __ctru_heap_size + 0x1000;

        // Init heap and newlib's syscalls
        initLib();

        // Init default settings
        FwkSettings &settings = FwkSettings::Get();

        settings.ThreadPriority = 0x30;
        settings.EcoMemoryMode = false;
        settings.AllowActionReplay = true;
        settings.AllowSearchEngine = true;
        settings.WaitTimeToBoot = Seconds(10.f);

        // Set default theme
        FwkSettings::SetThemeDefault();

        // If heap error, exit
        if (g_heapError)
            goto exit;

        // Init OSD hook
        OSDImpl::_Initialize();

        // Install CRO hook
        {
            const std::vector<u32> LoadCroPattern =
            {
                0xE92D5FFF, 0xE28D4038, 0xE89407E0, 0xE28D4054,
                0xE8944800, 0xEE1D4F70, 0xE59FC058, 0xE3A00000,
                0xE5A4C080, 0xE284C028, 0xE584500C, 0xE584A020
            };

            /*const std::vector<u32> UnloadCroPattern =
            {
                0xE92D4070, 0xEE1D4F70, 0xE59F502C, 0xE3A0C000,
                0xE5A45080, 0xE2846004, 0xE5840014, 0xE59F001C,
                0xE886100E, 0xE5900000, 0xEF000032, 0xE2001102
            };*/

            u32     loadCroAddress = Utils::Search<u32>(0x00100000, Process::GetTextSize(), LoadCroPattern);

            if (loadCroAddress)
            {
                LightLock_Init(&g_onLoadCroLock);
                g_onLoadCroHook.Initialize(loadCroAddress, (u32)LoadCROHooked);
                g_onLoadCroHook.Enable();
            }
        }

        // Init sdmc & paths
        InitFS();

        // Init System::Heap
        InitHeap();

        // Patch process before it starts & let the dev init some settings
        PatchProcess(settings);

        // Continue game
        svcSignalEvent(g_continueGameEvent);

        // Check threads priorities
        settings.ThreadPriority = std::min(settings.ThreadPriority, (u32)0x3E);
        g_gspEventThreadPriority = settings.ThreadPriority + 1;

        // Wait for the required time
        Sleep(settings.WaitTimeToBoot);

        svcCreateEvent(&g_keepEvent, RESET_ONESHOT);

        // Create plugin's main thread
        g_mainThread = threadCreate(ThreadInit, nullptr, (void *)threadStack, 0x4000, 0x18, 0);

        // Reduce priority
        while (R_FAILED(svcSetThreadPriority(g_keepThreadHandle, settings.ThreadPriority + 1)));

        // Wait until Main Thread finished all it's initializing
        svcWaitSynchronization(g_keepEvent, U64_MAX);
        svcClearEvent(g_keepEvent);

        if (settings.AllowActionReplay)
        {
            while (g_keepRunning)
            {
                // Wait for a new frame
                LightEvent_Wait(&OSDImpl::OnNewFrameEvent);

                // Lock the AR & execute codes before releasing it
                PluginMenuExecuteLoop::LockAR();
                PluginMenuExecuteLoop::ExecuteAR();
                PluginMenuExecuteLoop::UnlockAR();
            }
        }
        else
        {
            while (g_keepRunning)
            {
                svcWaitSynchronization(g_keepEvent, U64_MAX);
                svcClearEvent(g_keepEvent);
            }
        }

        threadJoin(g_mainThread, U64_MAX);
        threadFree(g_mainThread);

    exit:
        svcCloseHandle(g_keepEvent);

        exit(0);
    }

    // Initialize most subsystem / Global variables
    void    Initialize(void)
    {
        // Init HID
        hidInit();

        // Init classes
        Font::Initialize();

        // Init event thread
        gspInit();

        // Init scheduler
        Scheduler::Initialize();

        {
            // If /cheats/ doesn't exists, create it
            const char *dirpath = "/cheats";
            if (!Directory::IsExists(dirpath))
                Directory::Create(dirpath);
        }

        // Set AR file path
        Preferences::CheatsFile = "cheats.txt";

        // Default: cheats.txt in cwd
        if (!File::Exists(Preferences::CheatsFile))
            Preferences::CheatsFile = Utils::Format("/cheats/%016llX.txt", Process::GetTitleID());

        {
            // If /Screenshots/ doesn't exists, create it
            const char *dirpath = "/Screenshots";
            if (!Directory::IsExists(dirpath))
                Directory::Create(dirpath);

            // Set default screenshot path
            Screenshot::Path = dirpath;
            Screenshot::Path.append("/");

            // Set default screenshot prefix
            Screenshot::Prefix = "[";
            Process::GetName(Screenshot::Prefix);
            Screenshot::Prefix += Utils::Format(" - %08X] - Screenshot", (u32)Process::GetTitleID());
            Screenshot::Initialize();
        }
    }

    // Main thread's start
    void  ThreadInit(void *arg)
    {
        Initialize();

        // Initialize Globals settings
        InitializeRandomEngine();

        // Wake up init thread
        svcSignalEvent(g_keepEvent);

        // Reduce thread priority
        svcSetThreadPriority(threadGetCurrent()->handle, FwkSettings::Get().ThreadPriority);

        // Start plugin
        main();

        ThreadExit();
    }

    void  ThreadExit(void)
    {
        // In which thread are we ?
        if (threadGetCurrent() == g_mainThread)
        {
            // ## MainThread ##

            // Remove the OSD Hook
            OSDImpl::OSDHook.Disable();

            // Release process in case it's currently paused
            ProcessImpl::IsPaused = std::min((u32)ProcessImpl::IsPaused, (u32)1);
            ProcessImpl::Play(false);

            // Exit services
            gspExit();

            // Exit loop in keep thread
            g_keepRunning = false;
            svcSignalEvent(g_keepEvent);

            threadExit(1);
            return;
        }

        // ## Primary Thread ##
        if (g_mainThread != nullptr)
        {
            ProcessImpl::Play(false);
            PluginMenuImpl::ForceExit();
            threadJoin(g_mainThread, U64_MAX);
        }
        else // We aborted in a very early stage, so just release the game and exit
            svcSignalEvent(g_continueGameEvent);
    }

    extern "C"
    int   LaunchMainThread(int arg)
    {
        // Create event
        svcCreateEvent(&g_continueGameEvent, RESET_ONESHOT);
        // Start ctrpf's primary thread
        svcCreateThread(&g_keepThreadHandle, KeepThreadMain, arg, (u32 *)&keepThreadStack[0x1000], 0x1A, 0);
        // Wait until basic initialization has been made before returning to game
        svcWaitSynchronization(g_continueGameEvent, U64_MAX);
        // Close the event
        svcCloseHandle(g_continueGameEvent);

        return (0);
    }
}
