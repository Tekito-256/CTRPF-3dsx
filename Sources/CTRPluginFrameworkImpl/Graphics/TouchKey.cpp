#include "CTRPluginFrameworkImpl/Graphics/TouchKey.hpp"
#include "CTRPluginFrameworkImpl/Preferences.hpp"
#include "ctrulib/util/utf.h"

namespace CTRPluginFramework
{
    TouchKey::TouchKey(int character, IntRect ui, bool isEnabled)
    {
        _character = character;
        _content = nullptr;
        _icon = nullptr;
        _uiProperties = ui;
        _enabled = isEnabled;

        _isPressed = false;
        _execute = false;
    }

    TouchKey::TouchKey(const std::string &str, IntRect ui, int value, bool isEnabled)
    {
        _character = value;
        _content = new KeyContent{ str, Renderer::GetTextSize(str.c_str()) };
        _icon = nullptr;
        _uiProperties = ui;
        _enabled = isEnabled;

        _isPressed = false;
        _execute = false;
    }

    TouchKey::TouchKey(int value, IconCallback icon, IntRect ui, bool isEnabled)
    {
        _character = value;
        _content = nullptr;
        _icon = icon;
        _uiProperties = ui;
        _enabled = isEnabled;

        _isPressed = false;
        _execute = false;
    }

    TouchKey::~TouchKey()
    {
    }

    void TouchKey::Clear(void)
    {
        if (_content != nullptr)
            delete _content;
    }

    void    TouchKey::Enable(bool isEnabled)
    {
        _enabled = isEnabled;
    }

    void    TouchKey::ChangeContent(const std::string &content)
    {
        if (_content != nullptr)
        {
            _content->text = content;
            _content->width = Renderer::GetTextSize(content.c_str());
        }
    }

    void    TouchKey::DrawCharacter(const IntRect &rect, const Color &color)
    {
        // If not a string
        if (_content == nullptr)
        {
            char c = static_cast<char>(_character);
            Glyph *glyph = Font::GetGlyph(c);

            if (glyph == nullptr)
                return;

            int posX = ((rect.size.x - static_cast<int>(glyph->Width())) / 2) + rect.leftTop.x;
            int posY = ((rect.size.y - 16) / 2) + rect.leftTop.y;

            Renderer::DrawGlyph(glyph, posX, posY, color);
        }
        // String
        else
        {
            int posX = ((rect.size.x - _content->width) / 2) + rect.leftTop.x;
            int posY = ((rect.size.y - 16) / 2) + rect.leftTop.y;

            u8  *str = reinterpret_cast<u8 *>(const_cast<char *>(_content->text.c_str()));
            do
            {
                Glyph *glyph = Font::GetGlyph(str);

                if (glyph == nullptr)
                    break;

                posX = Renderer::DrawGlyph(glyph, posX, posY, color);
            } while (*str);
        }

    }

    void    TouchKey::Draw(void)
    {
        const auto      &theme = Preferences::Settings.Keyboard;
        const Color     &background = _isPressed ? theme.KeyBackgroundPressed : theme.KeyBackground;
        const Color     &text = _isPressed ? theme.KeyTextPressed : theme.KeyText;

        // Background
        Renderer::DrawRect(_uiProperties, background);

        // Icon
        if (_icon != nullptr)
        {
            int     posX = ((_uiProperties.size.x - 15) / 2) + _uiProperties.leftTop.x;
            int     posY = ((_uiProperties.size.y - 15) / 2) + _uiProperties.leftTop.y;

            _icon(posX, posY, false);
        }
        // Character
        else
            DrawCharacter(_uiProperties, text);
    }

    void    TouchKey::Update(const bool isTouchDown, const IntVector &touchPos)
    {
        if (!_enabled)
            return;

        bool    isTouched = _uiProperties.Contains(touchPos);

        if (_isPressed && !isTouchDown)
        {
            _isPressed = false;
            _execute = true;
        }

        _isPressed = isTouchDown && isTouched;
    }

    int      TouchKey::operator()(std::string &str)
    {
        // Hacky code for BACKSPACE key holding
        if (_enabled && _character == 0x8)
        {
            if (_isPressed)
                return (_character);
            if (_execute)
            {
                _execute = false;
                return (~_character);
            }
        }
        else
        {
            if (_enabled && _execute)
            {
                _execute = false;
                if (_content != nullptr && _character == 0x12345678)
                {
                    str += _content->text;
                }
                return (_character);
            }
        }

        return (-1);
    }
}
