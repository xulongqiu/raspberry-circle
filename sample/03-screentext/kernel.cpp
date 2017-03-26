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
static CDevice *pTarget = 0;

CKernel::CKernel (void)
:	m_Memory (TRUE),
	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Logger (m_Options.GetLogLevel ())
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}
	
	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}
	
	if (bOK)
	{
		pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
	}
	
	return bOK;
}

TShutdownMode CKernel::Run (void)
{
    char buf[128] = {0};
    char* buf_wp = buf;
    char* buf_ep = buf + 127;

	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);
	// show the character set on screen
	for (char chChar = ' '; chChar <= '~'; chChar++)
	{
		if (chChar % 8 == 0)
		{
			m_Screen.Write ("\n", 1);
		}

		CString Message;
		Message.Format ("%02X: \'%c\' ", (unsigned) chChar, chChar);
		
		m_Screen.Write ((const char *) Message, Message.GetLength ());
	}
	m_Screen.Write ("\n", 1);

    while(1){
        if(pTarget->Read(buf_wp, 1) <= 0)
            continue;
        
        if((buf_wp + 1) > buf_ep){
            m_Logger.Write(FromKernel, LogError, "command is too long! max_len(cmd) = 127");
            halt();
        }
        if(*buf_wp == '\n' || *buf_wp == '\r'){
            *buf_wp = '\n';
            *(++buf_wp) = 0;
            m_Logger.Write(FromKernel, LogNotice, "Command:%s", buf);

            if(strcmp(buf, "reboot\n") == 0){
                reboot();
            }   
            buf_wp = buf;
        }else{
            buf_wp++;
        }
    }

#ifndef NDEBUG
	// some debugging features
	m_Logger.Write (FromKernel, LogDebug, "Dumping the start of the ATAGS");
	debug_hexdump ((void *) 0x100, 128, FromKernel);

	m_Logger.Write (FromKernel, LogNotice, "The following assertion will fail");
	assert (1 == 2);
#endif

	return ShutdownHalt;
}
