#ifndef CTRPLUGINFRAMEWORK_MENUFOLDER_HPP
#define CTRPLUGINFRAMEWORK_MENUFOLDER_HPP

#include "CTRPluginFramework/MenuItem.hpp"
#include <vector>

namespace CTRPluginFramework
{
    class MenuFolder : public MenuItem
    {
    public:
        MenuFolder(std::string name, std::string note = "");
        ~MenuFolder();

        void    Append(MenuItem *item);
        u32     ItemsCount(void);

    private:
        friend class Menu;

        // Private methods
        void            _Open(MenuFolder *parent, int position);
        MenuFolder      *_Close(int &position);

        // Private members
        MenuFolder              *_parent;
        std::vector<MenuItem *>   _items;
        int                     _position;

    };
}

#endif