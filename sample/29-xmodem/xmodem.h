//
// kernel.h
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
#ifndef _xmodem_h
#define _xmodem_h

#include <circle/memory.h>
#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/logger.h>
#include <circle/types.h>

enum XmodemStates 
{
    IDLE,
    WSTART, 
    START, 
    SEQ, 
    CSEQ, 
    DATA, 
    DEND
};

typedef enum XmodemRtn
{
    XmodemRtnStateErr,
    XmodemRtnNoTrans,
    XmodemRtnPacket,
    XmodemRtnSeqErr,
    XmodemRtnCseqErr,
    XmodemRtnFinish
}XmodemRtn;

#define SOH             0x01
#define STX             0x02
#define EOT             0x04
#define ACK             0x06
#define NAK             0x15
#define CAN             0x18
#define CTRLZ           0x1A

#define XMODEM_PACKET_DATA_LEN 128
#define XMODEM_PACKAT_LEN 132

class CXmodem
{
public:
	CXmodem (void);
	~CXmodem (void);

	boolean Initialize (CSerialDevice* serial);
    void Start(void);
    void Ack(void);
    void SendDataPacket(const unsigned char* buf, unsigned short len);
    XmodemRtn RevDataPacket(unsigned char* buf);
    
private:
	void SendCmd(unsigned char cmd);
private:
	// do not change this order
	CMemorySystem		m_Memory;
	CActLED			m_ActLED;
	CSerialDevice*		m_Serial;
    unsigned char m_RevSeq;
    unsigned char m_SndSeq;
    enum XmodemStates m_XmodemState;
};

#endif
