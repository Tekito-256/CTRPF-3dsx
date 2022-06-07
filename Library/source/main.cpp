#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>

#include "CTRPluginFramework.hpp"

namespace CTRPluginFramework {
    int     main(void)
    {
        PluginMenu *menu = new PluginMenu("Action Replay", 0, 7, 3);

        menu->SynchronizeWithFrame(true);
        menu->Run();

        delete menu;
        return (0);
    }
}
