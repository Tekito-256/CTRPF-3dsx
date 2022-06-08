#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>

#include "CTRPluginFramework.hpp"

namespace CTRPluginFramework {

    void TestFunc(MenuEntry *e)
    {
        if(Controller::IsKeyPressed(Key::A))
          OSD::Notify("Hello world!");
    }

    void MenuFrameCb(Time t)
    {
        auto &scr = OSD::GetTopScreen();

        scr.DrawRect(30, 0, 340, 20, Color::Red);
    }

    int     main(void)
    {
        PluginMenu *menu = new PluginMenu("Action Replay", 0, 7, 3);

        *menu += new MenuEntry("Test cheat", TestFunc);

        menu->OnNewFrame = MenuFrameCb;

        menu->SynchronizeWithFrame(true);
        menu->Run();

        delete menu;
        return (0);
    }
}
