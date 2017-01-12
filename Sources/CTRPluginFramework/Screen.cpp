#include "CTRPluginFramework.hpp"
#include "3DS.h"
#include <new>

namespace CTRPluginFramework
{

    #define REG(x) *(vu32 *)(x)

    Screen *Screen::Top = NULL;
    Screen  *Screen::Bottom = NULL;

    u32     FromPhysicalToVirtual(u32 address)
    {
        if (address >= 0x18000000 && address <= 0x20000000)
            address += 0x07000000;
        else if (address >= 0x20000000 && address <= 0x30000000)
        {
            u32     values[2] = {0};

            svcGetProcessInfo((s64 *)&values, Process::GetHandle(), 20);
            if (values[0] == 0xF0000000)
                address += 0x10000000;
            else
                address -= values[0];
        }
        else
            return (0);
        return (address);
    }

    u32 GetBPP(GSPGPU_FramebufferFormats format)
    {
        switch(format) 
        {
            case GSP_RGBA8_OES:
                return 4;
            case GSP_BGR8_OES:
                return 3;
            case GSP_RGB565_OES:
            case GSP_RGB5_A1_OES:
            case GSP_RGBA4_OES:
                return 2;
        }
        return 3;
    }

    Screen::Screen(u32 lcdSetupInfo, u32 fillColorAddress, bool isTopScreen) : 
    _LCDSetup(lcdSetupInfo),
    _FillColor(fillColorAddress),
    _isTopScreen(isTopScreen)
    {
        // Get format
        _format = REG(_LCDSetup + LCDSetup::Format);

        // Get width & height
        u32 wh = REG(_LCDSetup + LCDSetup::WidthHeight);
        _width = (u16)(wh & 0xFFFF);
        _height = (u16)(wh >> 16);

        // Get left framebuffers pointers (Physical need to be converted)
        _leftFramebuffersP[0] = REG(_LCDSetup + LCDSetup::FramebufferA1);
        _leftFramebuffersP[1] = REG(_LCDSetup + LCDSetup::FramebufferA2);

        // Converting the framebuffers
        _leftFramebuffersV[0] = FromPhysicalToVirtual(_leftFramebuffersP[0]);
        _leftFramebuffersV[1] = FromPhysicalToVirtual(_leftFramebuffersP[1]);

        if (isTopScreen)
        {
            // Get right framebuffers pointers (Physical need to be converted)
            _rightFramebuffersP[0] = REG(_LCDSetup + LCDSetup::FramebufferB1);
            _rightFramebuffersP[1] = REG(_LCDSetup + LCDSetup::FramebufferB2);  

            // Converting the framebuffers
            _rightFramebuffersV[0] = FromPhysicalToVirtual(_rightFramebuffersP[0]);
            _rightFramebuffersV[1] = FromPhysicalToVirtual(_rightFramebuffersP[1]);
        }
        else
        {
            _rightFramebuffersP[0] = 0;
            _rightFramebuffersP[1] = 0;
            _rightFramebuffersV[0] = 0;
            _rightFramebuffersV[1] = 0;
        }


        // Set current buffer pointers
        _currentBuffer = _LCDSetup + LCDSetup::Select;

        // Get stride
        _stride = REG(_LCDSetup + LCDSetup::Stride);

        // Set bytes per pixel
        _bytesPerPixel = GetBPP(GetFormat());

        // Set row size
        _rowSize = _stride / _bytesPerPixel;

    }

    void    Screen::Initialize(void)
    {
        Screen::Top = new Screen(System::GetIOBasePDC() + 0x400, System::GetIOBaseLCD() + 0x204, true);
        Screen::Bottom = new Screen(System::GetIOBasePDC() + 0x500, System::GetIOBaseLCD() + 0xA04);
    }

    bool    Screen::IsTopScreen(void)
    {
        return (_isTopScreen);
    }

    bool    Screen::Is3DEnabled(void)
    {
        if (!_isTopScreen)
            return (false);

        return (_format & 0b100000 ? true : false);
    }

    void    Screen::Flash(Color &color)
    {
        u32     fillColor = (color.ToU32() & 0xFFFFFF) | 0x01000000;

        for (int i = 0; i < 0x64; i++)
        {
            REG(_FillColor) = fillColor;
            svcSleepThread(5000000); // 0.005 second
        }
        REG(_FillColor) = 0;
    }

    void    Screen::SwapBuffer(void)
    {
        REG(_currentBuffer) = !REG(_currentBuffer);
    }

    GSPGPU_FramebufferFormats   Screen::GetFormat(void)
    {
        return ((GSPGPU_FramebufferFormats)(_format & 0b111));
    }

    u16     Screen::GetWidth(void)
    {
        return (_width);
    }

    u16     Screen::GetHeight(void)
    {
        return (_height);
    }

    u32     Screen::GetStride(void)
    {
        return (_stride);
    }

    u32     Screen::GetRowSize(void)
    {
        return (_rowSize);
    }

    u32     Screen::GetBytesPerPixel(void)
    {
        return (_bytesPerPixel);
    }

    void    Screen::Update(void)
    {
        RefreshFramebuffers();
    }

    void    Screen::RefreshFramebuffers(void)
    {
        u32     leftFB = {0};

        leftFB = REG(_LCDSetup + FramebufferA1);

        if (leftFB == _leftFramebuffersP[0])
            return;

        // Get format
        _format = REG(_LCDSetup + LCDSetup::Format);

        // Get width & height
        u32 wh = REG(_LCDSetup + LCDSetup::WidthHeight);
        _width = (u16)(wh & 0xFFFF);
        _height = (u16)(wh >> 16);

        // Get stride
        _stride = REG(_LCDSetup + LCDSetup::Stride);

        // Set bytes per pixel
        _bytesPerPixel = GetBPP(GetFormat());

        // Set row size
        _rowSize = _stride / _bytesPerPixel;


        _leftFramebuffersP[0] = REG(_LCDSetup + FramebufferA1);
        _leftFramebuffersP[1] = REG(_LCDSetup + FramebufferA2);
        _leftFramebuffersV[0] = FromPhysicalToVirtual(_leftFramebuffersP[0]);
        _leftFramebuffersV[1] = FromPhysicalToVirtual(_leftFramebuffersP[1]);

        if (!_isTopScreen)
            return;

        _rightFramebuffersP[0] = REG(_LCDSetup + FramebufferB1);
        _rightFramebuffersP[1] = REG(_LCDSetup + FramebufferB2);
        _rightFramebuffersV[0] = FromPhysicalToVirtual(_rightFramebuffersP[0]);
        _rightFramebuffersV[1] = FromPhysicalToVirtual(_rightFramebuffersP[1]);
    }

    u8      *Screen::GetLeftFramebuffer(bool current)
    {
        u32    index = REG(_currentBuffer) & 0b1;

        if (current)
        {
            return ((u8 *)_leftFramebuffersV[index]); 
        }
        return ((u8 *)_leftFramebuffersV[!index]);            
    }

    u8      *Screen::GetRightFramebuffer(bool current)
    {
        if (!_isTopScreen)
            return (nullptr);

        u32    index = REG(_currentBuffer) & 0b1;

        if (current)
        {
            return ((u8 *)_rightFramebuffersV[index]); 
        }
        return ((u8 *)_rightFramebuffersV[!index]);            
    }
}