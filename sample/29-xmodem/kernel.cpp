//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "kernel.h"
#include <circle/string.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>
#include <circle/startup.h>

static const char FromKernel[] = "kernel";
static CDevice* pTarget = 0;

CKernel::CKernel(void)
    :   m_Memory(TRUE),
        m_Screen(m_Options.GetWidth(), m_Options.GetHeight()),
        m_Logger(m_Options.GetLogLevel())
{
    m_ActLED.Blink(5);  // show we are alive
}

CKernel::~CKernel(void)
{
}

boolean CKernel::Initialize(void)
{
    boolean bOK = TRUE;

    if (bOK) {
        bOK = m_Screen.Initialize();
    }

    if (bOK) {
        bOK = m_Serial.Initialize(115200);
    }

    if (bOK) {
        bOK = m_xmodem.Initialize(&m_Serial);
    }

    if (bOK) {
        pTarget = m_DeviceNameService.GetDevice(m_Options.GetLogDevice(), FALSE);

        if (pTarget == 0) {
            pTarget = &m_Screen;
        }

        bOK = m_Logger.Initialize(pTarget);
    }

    return bOK;
}


void delay(int ms)
{
    int i = 0, j = 0;
    int delay;

    for (i = 0; i < 100; i++) {
        for (j = 0; j < 10000; j++) {
            delay = ms;

            while (delay != 0) {
                delay --;
            }
        }
    }
}

TShutdownMode CKernel::Run(void)
{
    char buf[128] = {0};
    char* buf_wp = buf;
    char* buf_ep = buf + 127;
    unsigned char rev_len = 0;
    unsigned char xmode_buf[256] = {0};
    XmodemRtn xmodem_rtn = XmodemRtnPacket;

    m_Logger.Write(FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

    // show the character set on screen
    for (char chChar = ' '; chChar <= '~'; chChar++) {
        if (chChar % 8 == 0) {
            m_Screen.Write("\n", 1);
        }

        CString Message;
        Message.Format("%02X: \'%c\' ", (unsigned) chChar, chChar);

        m_Screen.Write((const char*) Message, Message.GetLength());
    }

    m_Screen.Write("\n", 1);

    while (1) {
        rev_len = buf_ep - buf_wp;
        rev_len = pTarget->Read(buf_wp, rev_len);

        if (rev_len <= 0) {
            continue;
        }

        buf_wp += rev_len - 1;

        if (*buf_wp == '\n' || *buf_wp == '\r') {
            *buf_wp = '\n';
            *(++buf_wp) = 0;
            m_Logger.Write(FromKernel, LogNotice, "Command:%s", buf);

            if (strcmp(buf, "reboot\n") == 0) {
                reboot();
            } else if (strcmp(buf, "update\n") == 0) {
                m_xmodem.Start();

                while (xmodem_rtn != XmodemRtnFinish) {
                    xmodem_rtn = m_xmodem.RevDataPacket(xmode_buf);

                    if (xmodem_rtn == XmodemRtnStateErr || xmodem_rtn == XmodemRtnNoTrans) {
                        m_Logger.Write(FromKernel, LogNotice, "NoFileTrans");
                        m_xmodem.Start();
                    } else if (xmodem_rtn == XmodemRtnPacket) {
                        m_Logger.Write(FromKernel, LogNotice, "RecOnePacket");
                        m_xmodem.Ack();
                    } else if (xmodem_rtn == XmodemRtnFinish) {
                        m_Logger.Write(FromKernel, LogNotice, "Xmodem Finish");
                    } else {
                        m_Logger.Write(FromKernel, LogNotice, "Xmodem Error");
                        break;
                    }
                }

                while (1) {
                    delay(50);
                    m_Logger.Write(FromKernel, LogNotice, "Error:%d", xmodem_rtn);
                }
            }

            buf_wp = buf;
        } else {
            buf_wp++;

            if (buf_wp >= buf_ep) {
                buf_wp = buf;
            }
        }
    }

#ifndef NDEBUG
    // some debugging features
    m_Logger.Write(FromKernel, LogDebug, "Dumping the start of the ATAGS");
    debug_hexdump((void*) 0x100, 128, FromKernel);

    m_Logger.Write(FromKernel, LogNotice, "The following assertion will fail");
    assert(1 == 2);
#endif

    return ShutdownHalt;
}
