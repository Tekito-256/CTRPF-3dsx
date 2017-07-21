#include "CTRPluginFramework/Graphics/OSD.hpp"
#include "CTRPluginFrameworkImpl/Graphics/OSDImpl.hpp"
#include "CTRPluginFrameworkImpl/Graphics/PrivColor.hpp"
#include "CTRPluginFrameworkImpl/System/Screen.hpp"
#include "font6x10Linux.h"
#include "CTRPluginFramework/System/Sleep.hpp"

namespace CTRPluginFramework
{

    int     OSD::Notify(std::string str, Color fg, Color bg)
    {
        OSDImpl *inst = OSDImpl::GetInstance();

        if (inst == nullptr)
            return (-1);

        while (inst->TryLock())
            Sleep(Microseconds(1));

        if (inst->_list.size() >= 50)
        {
            inst->Unlock();
            return (-1);
        }           

        inst->_list.push_back(OSDImpl::OSDMessage(str, fg, bg));
        inst->Unlock();
        return (0);
    }

    void    OSD::WriteLine(int screen, std::string line, int posX, int posY, Color fg, Color bg)
    {
		OSDImpl *inst = OSDImpl::GetInstance();

        if (inst == nullptr || line.size() == 0 || posX < 0 || posY < 0 || posY > 240)
            return;

        if (!screen) inst->_DrawBottom(line, posX, posY, fg, bg);
        else
        {
            Screen::Top->Acquire(true);
            inst->_DrawTop(line, posX, posY, 10, fg, bg);
            Screen::Top->Invalidate();
        }
    }

	void    OSD::WriteLine(int screen, std::string line, int posX, int posY, int offset, Color fg, Color bg)
	{
		OSDImpl *inst = OSDImpl::GetInstance();

		if (inst == nullptr || line.size() == 0 || posX < 0 || posY < 0 || posY > 240)
			return;

        if (!screen) inst->_DrawBottom(line, posX, posY, fg, bg);
        else
        {
            Screen::Top->Acquire(true);
            inst->_DrawTop(line, posX, posY, offset, fg, bg);
            Screen::Top->Invalidate();
        }
	}
}
