#ifndef CTRPLUGINFRAMEWORKIMPL_HEXEDITOR_HPP
#define CTRPLUGINFRAMEWORKIMPL_HEXEDITOR_HPP

#include "CTRPluginFrameworkImpl/Graphics.hpp"
#include "CTRPluginFrameworkImpl/System.hpp"
#include "CTRPluginFrameworkImpl/Menu/KeyboardImpl.hpp"
#include "CTRPluginFrameworkImpl/Menu/SubMenu.hpp"

#include <vector>
#include <string>

namespace CTRPluginFramework
{
    class HexEditor
    {
        using EventList = std::vector<Event>;

        enum
        {
            ByteV = 0,
            IntegerV = 1,
            //FloatV = 2,
            AsmV = 2
        };

        enum
        {
            DirtySrc = 1,
            DirtyMemory = 1 << 1,
            DirtyCursor = 1 << 2,

            DirtyViewCache = 1 << 3,
            DirtyCursorAddress = 1 << 4,
            DirtyCursorPos = 1 << 5,
            DirtyCursorCache = 1 << 6,

            InvalidSrc = 1 << 8,
        };

        struct Item
        {
            u32     address;
            union
            {
                u32     value32;
                float   valueF;
                u8      b[4];
            };

            union
            {
                u32     origin32;
                float   originF;
                u8      ob[4];
            };
            char    addrCache[9]{0};
            std::string valueCache;
            std::string dataCache;
        };

        struct Context
        {
            u32     _flags;
            u32     _address;
            u32     _cursorAddress;
            s16     _cursorX;
            s16     _cursorY;
            u16     _cursorPosX;
            u16     _cursorPosY;
            char    _cursorAddressStr[9]{0};

            Clock   _clock;
            Time    _latency{Milliseconds(200)};
            std::vector<Item>   _items;

            bool    ScrollUp(u32 stride, bool updateCursor);
            bool    ScrollDown(u32 stride, bool updateCursor);
        };

        struct IView
        {
            virtual void    Draw(void) = 0;
            virtual void    ProcessEvent(Event &event) = 0;
            virtual void    UpdateView(void) = 0;
            virtual void    EditValueAtCursor(u32 val) = 0;

            void    ProcessEventsCommon(Context& _ctx, Event& event, s16 maxX);

            u8      ItemsPerLine;   ///< Nb of items displayed on one line by the view
            u8      DigitPerLine;   ///< Nb of editable digit per line
            u8      TotalItems;     ///< Nb of items which can be displayed
            u8      Stride;         ///< Nb of bytes which must be sub/add to fetch a new line
        };

        struct ByteView : IView
        {
            ByteView(Context& ctx);

            void    Draw(void);
            void    ProcessEvent(Event &event);
            void    UpdateView(void);
            void    EditValueAtCursor(u32 val);

            Context&    _ctx;
        };

        struct IntegerView : IView
        {
            IntegerView(Context &ctx);

            void    Draw(void);
            void    ProcessEvent(Event &event);
            void    UpdateView(void);
            void    EditValueAtCursor(u32 val);

            Context&    _ctx;
        };

        struct FloatView : IView
        {
            FloatView(Context &ctx);

            void    Draw(void);
            void    ProcessEvent(Event &event);
            void    UpdateView(void);
            void    EditValueAtCursor(u32 val);

            Context&    _ctx;
        };

        struct AsmView : IView
        {
            AsmView(Context &ctx);

            void    Draw(void);
            void    ProcessEvent(Event &event);
            void    UpdateView(void);
            void    EditValueAtCursor(u32 val);

            Context&    _ctx;
        };

    public:
        HexEditor(u32 target);
        ~HexEditor();

        // Return true if the user decided to close it
        bool    operator()(EventList &eventList);
        bool    Goto(u32 address, bool updateCursor = false);

    private:

        void    _ProcessEvent(Event &event);
        static s32     _RenderTop(void *arg);
        void    _RenderBottom(void);
        void    _Update(void);

        void    _CreateCheat(void);

        void    _MoveBackward(void);
        void    _MoveForward(void);
        void    _SaveThisAddress(void);
        void    _BrowseHistory(void);
        void    _ClearHistory(void);
        void    _ApplyChanges(void);
        void    _DiscardChanges(void);
        u32     _PromptForAddress(int mode);
        void    _JumpTo(int mode);
        void    _GotoPreviousRegion(void);
        void    _GotoNextRegion(void);
        Item&   _GetSelectedItem(void);

        u32                 _viewId;
        IView            *  _viewCurrent;
        IView            *  _views[4];
        Context             _ctx;
        SubMenu             _submenu;
        KeyboardImpl        _keyboard;
        Task                _renderTask;
        int                 _indexHistory;
        std::vector<u32>    _history;
    };
}

#endif
