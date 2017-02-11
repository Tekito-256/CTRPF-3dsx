#include "CTRPluginFrameworkImpl/Menu/MenuItem.hpp"
#include "CTRPluginFrameworkImpl/MenuEntryImpl.hpp"
#include "CTRPluginFramework/MenuFolderImpl.hpp"
#include "CTRPluginFramework/MenuFolder.hpp"


namespace CTRPluginFramework
{
    MenuFolder::MenuFolder(std::string name, std::string note) :
        _item(new MenuFolderImpl(name, note))
    {

    }

    void    MenuFolder::Append(MenuEntry *item)
    {
        MenuEntryImpl *entry = item->_item;

        _item->Append(entry);
    }

    void    MenuFolder::Append(MenuFolder *item)
    {
        MenuFolderImpl *folder = item->_item;

        _item->Append(item);
    }

    u32    MenuFolder::ItemsCount(void)
    {
        return (_item->ItemsCount());
    }
}