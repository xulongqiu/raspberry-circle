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
#include "xmodem.h"
#include <assert.h>

CXmodem::CXmodem(void)
    :   m_Memory(TRUE)

{
}

CXmodem::~CXmodem(void)
{
}

boolean CXmodem::Initialize(CSerialDevice* serial)
{
    boolean bOK = TRUE;

    m_Serial = serial;
    m_XmodemState = IDLE;
    m_RevSeq = 0;
    m_SndSeq = 0;
    return bOK;
}

void CXmodem::SendCmd(unsigned char cmd)
{
    assert(m_Serial != 0);
    m_Serial->Write(&cmd, 1);
}

void CXmodem::Start(void)
{
    SendCmd(NAK);
    m_XmodemState = WSTART;
}

void CXmodem::Ack(void)
{
    SendCmd(ACK);
    m_XmodemState = WSTART;
}

XmodemRtn CXmodem::RevDataPacket(unsigned char* buf)
{
    unsigned char revByte = 0;
    unsigned short revDataLen = 0;
    unsigned char checkSum = 0;
    unsigned char* Xmodem_Buffer = buf;
    unsigned char xerror = 0;
    unsigned int noDataCnt = 0;

    if (m_XmodemState == IDLE) {
        return XmodemRtnStateErr;
    }

    assert(m_Serial != 0);
    assert(Xmodem_Buffer != 0);

    while (1) {

        if (m_Serial->Read(&revByte, 1) == 1) {
            switch (m_XmodemState) {
            case WSTART:      //当前正等待接收主机发送起始符号SOH
                if (revByte == SOH) {   //收到主机发送的SOH符号
                    m_XmodemState = START; //进入"SOH已经收到"状态
                    xerror = 0;
                } else if (revByte == EOT) { //收到主机发送的EOT符号
                    SendCmd(ACK);
                    m_XmodemState = IDLE;         //进入"空闲"状态
                    return XmodemRtnFinish;
                }

                break;

            case START://SOH已经收到,当前正等待接收期望的包序列号
                m_XmodemState = SEQ;    //进入"序列号已经收到"状态

                if (revByte != m_RevSeq) {     //不是期望的包序列号
                    xerror = 1;    //设置出错标志
                }

                break;

            case SEQ:       //包号已经收到,当前正等待接收包号补码
                m_XmodemState = CSEQ;     //进入"包号补码已收"状态

                if (revByte != (0xff - m_RevSeq)) { //不是期望的包号补码
                    xerror = 1;    //设置出错标志
                }

                break;

            case CSEQ:      //包号补码已经收到,当前正等待接收数据
                Xmodem_Buffer[0] = revByte;
                checkSum = revByte;
                revDataLen = 1;
                m_XmodemState = DATA;
                break;

            case DATA:                       //当前正等待接收数据
                if (revDataLen < 128) {
                    Xmodem_Buffer[revDataLen++] = revByte;
                    checkSum += revByte;

                    if (revDataLen == 128) {
                        m_XmodemState = DEND;
                    }
                }

                break;

            case DEND:      //数据包接收结束,当前正等待接收校验和
                if ((checkSum == revByte) && (xerror == 0)) {
                    m_RevSeq++;
                    return XmodemRtnPacket;
                } else {
                    SendCmd(NAK);    //要求主机重发SOH符号
                    m_XmodemState = WSTART;//回到"等待接收SOH"状态
                }

                break;

            default:
                break;
            }
        } else if (noDataCnt++ > 1000000) {
            return XmodemRtnNoTrans;
        }
    }
}

void CXmodem::SendDataPacket(const unsigned char* buf, unsigned short len)
{
    ;
}
