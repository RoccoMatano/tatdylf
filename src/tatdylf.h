////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2007-2023 Rocco Matano
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////
//
// tatdylf is a stripped-down Windows DHCP server whose sole task is to provide
// Basler GigE-Vision cameras with IP addresses from a class C private network.
// The limitation to this operational objective allows many simplifications
// that would be inadmissible in the general case. E.g. the processing of DHCP
// options and memory reservation for those can be reduced to just handling
// a handful of those.
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////

#include "tatdylf_ll.h"

////////////////////////////////////////////////////////////////////////////////

static const uint8_t  BOOTP_REQUEST  =   1;
static const uint8_t  BOOTP_REPLY    =   2;
static const uint32_t CHADDR_N32     =   4;
static const uint32_t NUM_CLIENTS    =  32;
static const uint32_t SERVER_PORT    =  67;
static const uint32_t CLIENT_PORT    =  68;
static const uint32_t DHCP_OPT_SIZE  = 128;  // min required for Basler cameras
static const uint32_t DHCP_COOKIE    = 0x63538263;
static const uint32_t CC_NET_MASK_LE = 0xffff0000; // 255.255.0.0
static const uint32_t CC_PREFIX_LE   = 0xc0a80000; // 192.168.0.0
static const uint32_t CC_SUB_MASK_LE = 0xffffff00; // class c subnet mask
static const uint32_t CC_SUB_MASK_BE = 0x00ffffff; // class c subnet mask


////////////////////////////////////////////////////////////////////////////////

struct Packet
{
    // https://tools.ietf.org/html/rfc2131
    uint8_t  op;
    uint8_t  htype;
    uint8_t  hlen;
    uint8_t  hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint32_t chaddr[CHADDR_N32]; // using uint32_t here !!!
    char     sname[64];
    uint8_t  file[128];
    uint32_t magic_cookie;
    uint8_t  options[DHCP_OPT_SIZE];
};

////////////////////////////////////////////////////////////////////////////////

struct Request
{
    union
    {
        Packet packet;
        char   buffer[sizeof(Packet)];
    };
    uint32_t server_ip;
    uint32_t requested_ip;
    uint8_t  request_msg;
    uint8_t  reply_msg;
};

////////////////////////////////////////////////////////////////////////////////

struct Client
{
    uint32_t expiry;
    uint32_t chaddr[CHADDR_N32];
};

////////////////////////////////////////////////////////////////////////////////

struct Config
{
    SOCKET   socket;
    uint32_t server_ip;
    uint32_t lease;
    uint32_t range_start;
    uint32_t range_end;
    Client   clients[NUM_CLIENTS];
};

////////////////////////////////////////////////////////////////////////////////

enum DHCP_MESSAGES
{
    DMSG_DISCOVER = 1,
    DMSG_OFFER    = 2,
    DMSG_REQUEST  = 3,
    DMSG_ACK      = 5,
    DMSG_NAK      = 6,
};

////////////////////////////////////////////////////////////////////////////////

enum DHCP_OPTIONS
{
    DOPT_PAD               =   0,
    DOPT_SUBNET_MASK       =   1,
    DOPT_REQUESTED_IP_ADDR =  50,
    DOPT_ADDR_LEASE_TIME   =  51,
    DOPT_MESSAGE_TYPE      =  53,
    DOPT_SERVER_IDENT      =  54,
    DOPT_END               = 255
};

////////////////////////////////////////////////////////////////////////////////

void send_console_to_tray(PCTSTR title, HICON icon);
void print_fmt(const char *fmt, ...);

////////////////////////////////////////////////////////////////////////////////
