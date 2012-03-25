/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * Changes:
 *
 *    2011-08-17 Gregg Hamilton for OpenHome (http://openhome.org/wiki/OhNet:What)
 *            Split file into header and source for compilation against C code.
 */

#ifndef IFADDRS_ANDROID_H_included
#define IFADDRS_ANDROID_H_included

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "global.h"

// A smart pointer that closes the given fd on going out of scope.
// Use this when the fd is incidental to the purpose of your function,
// but needs to be cleaned up on exit.

class ScopedFd {
public:
    explicit ScopedFd(int fd) : fd(fd) {
    }

    ~ScopedFd() {
        close(fd);
    }

    int get() const {
        return fd;
    }

private:
    int fd;

    // Disallow copy and assignment.
    ScopedFd(const ScopedFd&);
    void operator=(const ScopedFd&);
};

// Android (bionic) doesn't have getifaddrs(3)/freeifaddrs(3).
// We fake it here, so java_net_NetworkInterface.cpp can use that API
// with all the non-portable code being in this file.

// Source-compatible subset of the BSD struct.
struct ifaddrs {
    // Pointer to next struct in list, or NULL at end.
    struct ifaddrs* ifa_next;

    // Interface name.
    char* ifa_name;

    // Interface flags.
    unsigned int ifa_flags;

    // Interface address.
    struct sockaddr* ifa_addr;

    // Interface netmask;
    struct sockaddr* ifa_netmask;

    // Interface broadcast
    struct sockaddr* ifa_broadaddr;

#ifdef __cplusplus
    ifaddrs(ifaddrs* next)
    : ifa_next(next), ifa_name(NULL), ifa_flags(0), ifa_addr(NULL), ifa_netmask(NULL), ifa_broadaddr(NULL)
    {
    }

    ~ifaddrs() {
        delete ifa_next;
        delete[] ifa_name;
        delete ifa_addr;
        delete ifa_netmask;
        delete ifa_broadaddr;
    }

    // Sadly, we can't keep the interface index for portability with BSD.
    // We'll have to keep the name instead, and re-query the index when
    // we need it later.
    bool setNameAndFlagsByIndex(int interfaceIndex, int family) {
        // Get the name.
        //interfaceIndex = 0;
        char buf[IFNAMSIZ];
        char* name = if_indextoname(interfaceIndex, buf);
        if (name == NULL) {
            return false;
        }
        ifa_name = new char[strlen(name) + 1];
        strcpy(ifa_name, name);

        // Get the flags.
        ScopedFd fd(socket(AF_INET, SOCK_DGRAM, 0));
        if (fd.get() == -1) {
            return false;
        }

        ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strcpy(ifr.ifr_name, name);
        int rc = ioctl(fd.get(), SIOCGIFFLAGS, &ifr);
        if (rc == -1) {
            return false;
        }
        ifa_flags = ifr.ifr_flags;

        if (ioctl(fd.get(), SIOCGIFNETMASK, &ifr) < 0)
        {
            return false;
        }

        return true;
    }

    // Netlink gives us the address family in the header, and the
    // sockaddr_in or sockaddr_in6 bytes as the payload. We need to
    // stitch the two bits together into the sockaddr that's part of
    // our portable interface.
    void setAddress(int family, void* data, size_t byteCount, size_t prefixlen) {
        sockaddr_storage* ss = new sockaddr_storage;
        ss->ss_family = family;
		void *dst;
        if (family == AF_INET) {
            dst = &reinterpret_cast<sockaddr_in*>(ss)->sin_addr;
        } else if (family == AF_INET6) {
            dst = &reinterpret_cast<sockaddr_in6*>(ss)->sin6_addr;
        }
		else {
			return;
		}
        memcpy(dst, data, byteCount);
        ifa_addr = reinterpret_cast<sockaddr*>(ss);

        ss = new sockaddr_storage;
        ss->ss_family = family;
		size_t buflen = 4;
        if (family == AF_INET) {
            dst = &reinterpret_cast<sockaddr_in*>(ss)->sin_addr;
			buflen = 4;
        } else if (family == AF_INET6) {
            dst = &reinterpret_cast<sockaddr_in6*>(ss)->sin6_addr;
			buflen = 16;
        }
		else {
			return;
		}
		memset(dst, 0xff, byteCount);
		memset((char *)dst + (prefixlen/8), 0, (byteCount - prefixlen/8));
        ifa_netmask = reinterpret_cast<sockaddr*>(ss);

        ss = new sockaddr_storage;
        ss->ss_family = family;
        if (family == AF_INET) {
            dst = &reinterpret_cast<sockaddr_in*>(ss)->sin_addr;
			buflen = 4;
        } else if (family == AF_INET6) {
            dst = &reinterpret_cast<sockaddr_in6*>(ss)->sin6_addr;
			buflen = 16;
        }
		else {
			return;
		}

		memset(dst, 0xff, byteCount);
        memcpy(dst, data, prefixlen/8);
        ifa_broadaddr = reinterpret_cast<sockaddr*>(ss);

    }
    #endif
};

// FIXME: use iovec instead.
struct addrReq_struct {
    struct nlmsghdr netlinkHeader;
    struct ifaddrmsg msg;
};

#ifdef __cplusplus
inline bool sendNetlinkMessage(int fd, const void* data, size_t byteCount);
inline ssize_t recvNetlinkMessage(int fd, char* buf, size_t byteCount);
#endif

#ifdef __cplusplus
extern "C" {
#endif
// Source-compatible with the BSD function.
int getifaddrs(struct ifaddrs** result);

// Source-compatible with the BSD function.
void freeifaddrs(struct ifaddrs* addresses);
#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // IFADDRS_ANDROID_H_included
