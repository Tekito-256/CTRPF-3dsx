#include "types.h"
#include "ctrulib/svc.h"
#include "CTRPluginFramework/Time.hpp"

namespace CTRPluginFramework
{
    void    Sleep(Time sleepTime)
    {
        if (sleepTime > Time::Zero)
            svcSleepThread(sleepTime.AsMicroseconds() * 1000);
    }
}