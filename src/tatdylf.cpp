////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2007-2022 Rocco Matano
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

#include "tatdylf.h"

////////////////////////////////////////////////////////////////////////////////

static const uint32_t MAX_INTERFACES = 4;
static const char APPL[] = "tatdylf";

static uint32_t get_config(Config cfg[MAX_INTERFACES]);
static bool receive_request(Request *req, Config *cfg);
static bool send_reply(Request *req, Config *cfg);

static inline char* ip2string(uint32_t ip)
{
    in_addr inaddr;
    inaddr.S_un.S_addr = ip;
    return inet_ntoa(inaddr);
}

////////////////////////////////////////////////////////////////////////////////

static DWORD WINAPI run_dhcp(void* param)
{
    Config& cfg = *static_cast<Config*>(param);
    print_fmt("Host  : %s\n", ip2string(cfg.server_ip));
    print_fmt("Range : %s - ", ip2string(htonl(cfg.range_start)));
    print_fmt("%s\n", ip2string(htonl(cfg.range_end)));
    print_fmt("Lease : %u\n\n", cfg.lease);

    for(;;)
    {
        Request req;
        if (receive_request(&req, &cfg))
        {
            if (send_reply(&req, &cfg))
            {
                SYSTEMTIME st;
                GetLocalTime(&st);
                print_fmt("Allotted %s to ", ip2string(req.packet.yiaddr));
                static const int MAC_SIZE = 6;
                uint8_t *m = reinterpret_cast<uint8_t*>(&req.packet.chaddr[0]);
                for (int i = 0; i < MAC_SIZE; i++)
                {
                    print_fmt("%02X:", m[i]);
                }
                print_fmt(
                    "\b for %us at %2d:%02d:%02d\n",
                    cfg.lease,
                    st.wHour,
                    st.wMinute,
                    st.wSecond
                    );
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void entry_point()
{
    Config cfg[MAX_INTERFACES];
    uint32_t num_good = get_config(cfg);
    if (num_good > 0)
    {
        send_console_to_tray(APPL, LoadIcon(GetModuleHandle(nullptr), APPL));
        for (uint32_t idx = 1; idx < num_good; idx++)
        {
            CloseHandle(
                CreateThread(
                    nullptr,
                    0,
                    run_dhcp,
                    static_cast<void*>(&cfg[idx]),
                    0,
                    nullptr
                    )
                );
        }
        run_dhcp(&cfg[0]);
    }
    else
    {
        print_fmt("invalid config\n");
        ExitProcess(1);
    }
}

////////////////////////////////////////////////////////////////////////////////

static uint8_t *extract_option(uint8_t *src, Option *opt)
{
    if (*src != DOPT_END)
    {
        opt->tag = *src++;
        opt->size = *src++;
        mem_cpy(opt->buf, src, opt->size);
        src += opt->size;
        return src;
    }
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

bool receive_request(Request *req, Config *cfg)
{
    zero_init(*req);

    sockaddr from;
    int from_len = sizeof(from);
    int size = recvfrom(
        cfg->socket,
        req->buffer,
        sizeof(Packet),
        0,
        &from,
        &from_len
        );
    if (size == SOCKET_ERROR)
    {
        print_fmt("rr error: %d\n", WSAGetLastError());
        return false;
    }

    const uint8_t req_op = req->packet.op;
    const uint32_t cookie = htonl(req->packet.magic_cookie);
    if (req_op != BOOTP_REQUEST || cookie != DHCP_COOKIE)
    {
        print_fmt("not DHCP: %u, %x\n", req_op, cookie);
        return false;
    }

    Option opt;
    uint8_t *src = req->packet.options;
    uint8_t * const end = src + sizeof(req->packet.options);
    while (src < end && (src = extract_option(src, &opt)) != nullptr)
    {
        switch (opt.tag)
        {
        case DOPT_MESSAGE_TYPE:
            req->request_msg = opt.buf[0];
            break;

        case DOPT_SERVER_IDENT:
            req->server_ip = opt.u32;
            break;

        case DOPT_REQUESTED_IP_ADDR:
            req->requested_ip = opt.u32;
            break;
        }
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

static uint32_t seconds_since_start()
{
    // There is NO overflow problem here! 'seconds_since_start' will deliver
    // continuing one second increments for approx. 136 years.
    static uint32_t first = 0;
    union ftu
    {
        FILETIME ft;
        uint64_t u;
    } ft;
    GetSystemTimeAsFileTime(&ft.ft);
    uint32_t now = static_cast<uint32_t>(ft.u / 10000000ULL);
    if (first == 0)
    {
        first = now;
    }
    return now - first;
}

////////////////////////////////////////////////////////////////////////////////

static inline bool equal_chaddr(const uint32_t *a, const uint32_t *b)
{
    // Here we rely on the fact that chaddr is at least 8 bytes in size
    // (actually the size is 16), but that only 6 bytes of an ethernet MAC are
    // stored there and that these are padded with zeros. Since we defined
    // a chaddr as an array of uint32_t, there are no alignment faults here.
    return (a[0] == b[0]) && (a[1] == b[1]);
}

////////////////////////////////////////////////////////////////////////////////

static uint32_t assign_address(Request *req, Config *cfg)
{
    uint32_t empty[CHADDR_N32];
    zero_init(empty);

    uint32_t now = seconds_since_start();

    const int num_addr = cfg->range_end - cfg->range_start + 1;
    int expired = -1;
    int i = 0;

    for (; i < num_addr; i++)
    {
        if (
            equal_chaddr(req->packet.chaddr, cfg->clients[i].chaddr) ||
            equal_chaddr(cfg->clients[i].chaddr, empty)
            )
        {
            // if this entry is unused or it is already reserved for the
            // current client, we use it
            break;
        }

        if (
            !equal_chaddr(cfg->clients[i].chaddr, empty) &&
            cfg->clients[i].expiry < now
            )
        {
            // if this entry was once used, but its lease has run out,
            // we memorize it to reuse it if necessary
            expired = i;
        }
    }

    if (i >= num_addr)
    {
        // no unused or reserved entry found
        if (expired >= 0)
        {
            // but we can reuse an expired one
            i = expired;
        }
        else
        {
            print_fmt("no available IP\n");
            return 0;
        }
    }

    mem_cpy(cfg->clients[i].chaddr, req->packet.chaddr, sizeof(empty));
    cfg->clients[i].expiry = now + 42;
    return htonl(cfg->range_start + i);
}

////////////////////////////////////////////////////////////////////////////////

static int client_index_from_ip(Config *cfg, uint32_t ip)
{
    uint32_t ip_host_end = htonl(ip);
    if (ip_host_end >= cfg->range_start && ip_host_end <= cfg->range_end)
    {
        return (ip_host_end - cfg->range_start);
    }
    else
    {
        return -1;
    }
}

////////////////////////////////////////////////////////////////////////////////

static inline int matching_client(uint32_t ip, uint32_t *chaddr, Config *cfg)
{
    int idx = client_index_from_ip(cfg, ip);
    if (idx >= 0)
    {
        if (equal_chaddr(chaddr, cfg->clients[idx].chaddr))
        {
            return idx;
        }
    }
    return -1;
}

////////////////////////////////////////////////////////////////////////////////

static uint8_t *attach_option(uint8_t *dst, const Option *opt)
{
    *dst++ = opt->tag;
    *dst++ = opt->size;
    mem_cpy(dst, opt->buf, opt->size);
    dst += opt->size;
    return dst;
}

////////////////////////////////////////////////////////////////////////////////

static int finalize_reply(Request *req, Config *cfg)
{
    zero_init(req->packet.options);
    uint8_t *dst = req->packet.options;

    Option opt;
    opt.tag = DOPT_MESSAGE_TYPE;
    opt.size = 1;
    opt.buf[0] = req->reply_msg;
    dst = attach_option(dst, &opt);

    if (req->reply_msg != DMSG_NAK)
    {
        opt.tag = DOPT_SUBNET_MASK;
        opt.size = 4;
        opt.u32 = CC_SUB_MASK_BE;
        dst = attach_option(dst, &opt);

        opt.tag = DOPT_SERVER_IDENT;
        opt.u32 = cfg->server_ip;
        dst = attach_option(dst, &opt);

        opt.tag = DOPT_ADDR_LEASE_TIME;
        opt.u32 = htonl(cfg->lease);
        dst = attach_option(dst, &opt);
    }

    *dst++ = DOPT_END;
    req->packet.op = BOOTP_REPLY;
    return static_cast<int>(dst - reinterpret_cast<uint8_t*>(&req->packet));
}

////////////////////////////////////////////////////////////////////////////////

bool send_reply(Request *req, Config *cfg)
{
    int client2update = -1;
    req->reply_msg = DMSG_NAK;
    req->packet.yiaddr = 0;

    if (req->request_msg == DMSG_DISCOVER)
    {
        req->packet.yiaddr = assign_address(req, cfg);
        if (req->packet.yiaddr)
        {
            req->reply_msg = DMSG_OFFER;
        }
    }
    else if (req->request_msg == DMSG_REQUEST)
    {
        if (req->server_ip == 0 || req->server_ip == cfg->server_ip)
        {
            const uint32_t ip = (
                req->packet.ciaddr ? req->packet.ciaddr : req->requested_ip
                );
            if (ip)
            {
                client2update = matching_client(ip, req->packet.chaddr, cfg);
                if (client2update >= 0)
                {
                    req->reply_msg = DMSG_ACK;
                    req->packet.yiaddr = ip;
                }
            }
        }
    }
    else
    {
        // no reply for unhandled messages
        return false;
    }

    sockaddr_in to;
    to.sin_family = AF_INET;
    to.sin_port = htons(CLIENT_PORT);
    to.sin_addr.s_addr = INADDR_BROADCAST;

    int size = finalize_reply(req, cfg);
    size = sendto(
        cfg->socket,
        req->buffer,
        size,
        0,
        reinterpret_cast<sockaddr*>(&to),
        sizeof(to)
        );
    if (size == SOCKET_ERROR)
    {
        print_fmt("sr error %d\n", WSAGetLastError());
    }

    if (size > 0 && client2update >= 0)
    {
        uint32_t t = seconds_since_start();
        cfg->clients[client2update].expiry = (
            (UINT32_MAX - t > cfg->lease) ?
            t + cfg->lease :
            UINT32_MAX
            );
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

static inline DWORD get_ini_string(
    const char *section,
    const char *key,
    char *str,
    uint32_t size,
    const char *filename
    )
{
    return GetPrivateProfileString(
        section,
        key,
        "",
        str,
        size,
        filename
        );
}

////////////////////////////////////////////////////////////////////////////////

static bool get_single_config(Config *cfg, const char *section, const char* ini)
{

    zero_init(*cfg);

    ///////////////////////////// server ip ////////////////////////////////////

    uint32_t server_ip_host_end;
    char str[256];
    if (get_ini_string(section, "ip", str, sizeof(str), ini))
    {
        cfg->server_ip = inet_addr(str);
        server_ip_host_end = htonl(cfg->server_ip);
        if ((server_ip_host_end & CC_NET_MASK_LE) != CC_PREFIX_LE)
        {
            print_fmt("not class C private: %s\n", str);
            return false;
        }
    }
    else
    {
        return false;
    }

    ////////////////////////////// ip range ////////////////////////////////////

    if ((server_ip_host_end & ~CC_SUB_MASK_LE) >= 128)
    {
        cfg->range_start = (server_ip_host_end & CC_SUB_MASK_LE) | 1;
        cfg->range_end = server_ip_host_end - 1;
    }
    else
    {
        cfg->range_start = server_ip_host_end + 1;
        cfg->range_end = (server_ip_host_end & CC_SUB_MASK_LE) | 254;
    }
    if ((cfg->range_end - cfg->range_start + 1) > NUM_CLIENTS)
    {
        cfg->range_end = cfg->range_start + NUM_CLIENTS - 1;
    }

    ///////////////////////////// lease time ///////////////////////////////////

#ifdef TATDYLF_READ_LEASE_TIME
    if (get_ini_string(section, "lease", str, sizeof(str), ini))
    {
        char *p = str;
        while (*p == ' ') p++;
        uint32_t accu = 0;
        char c;
        while ((c = *p++) != 0)
        {
            accu = accu * 10 + (c - '0');
        }
        cfg->lease = accu;
    }
    if (!cfg->lease)
    {
        cfg->lease = UINT32_MAX;
    }
#else
    cfg->lease = 600;
#endif

    /////////////////////////////// socket /////////////////////////////////////

    cfg->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (cfg->socket == INVALID_SOCKET)
    {
        print_fmt("failed to create socket\n");
        return false;
    }

    BOOL opt_val = true;
    int err = setsockopt(
        cfg->socket,
        SOL_SOCKET,
        SO_BROADCAST,
        reinterpret_cast<char*>(&opt_val),
        sizeof(BOOL)
        );
    if (err != SOCKET_ERROR)
    {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(SERVER_PORT);
        addr.sin_addr.s_addr = cfg->server_ip;

        err = bind(
            cfg->socket,
            reinterpret_cast<sockaddr*>(&addr),
            sizeof(addr)
            );
    }
    if (err == SOCKET_ERROR)
    {
        err = WSAGetLastError();
        print_fmt("error %d\n", err);
        closesocket(cfg->socket);
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

uint32_t get_config(Config cfg[MAX_INTERFACES])
{
    uint32_t num_good = 0;
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        print_fmt("no winsock\n");
        return num_good;
    }

    char ini_file[MAX_PATH + 1];
    GetModuleFileName(nullptr, ini_file, MAX_PATH);
    int len = sz_len(ini_file);
    while (len && ini_file[len] != '.') --len;
    sz_cpy(&ini_file[len + 1], "ini");

    for (uint32_t idx = 0; idx < MAX_INTERFACES; idx++)
    {
        char section[32];
        wsprintf(section, "iface%u", idx);
        if (get_single_config(&cfg[idx], section, ini_file))
        {
            num_good++;
        }
        else
        {
            break;
        }
    }

    return num_good;
}

////////////////////////////////////////////////////////////////////////////////
