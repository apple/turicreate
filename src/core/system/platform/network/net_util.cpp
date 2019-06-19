/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sstream>

#ifndef _WIN32
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif

#include <core/logging/assertions.hpp>
#include <network/net_util.hpp>
#include <core/export.hpp>

#define NUM_VM_MAC_ADDRS 8
#define MAC_PREFIX_SIZE 3
namespace turi {

bool str_to_ip(const char* c, uint32_t& out) {
  if (c == NULL) return false;
  else return inet_pton(AF_INET, c, &out) > 0;
}

bool ip_to_str(uint32_t ip, std::string& out) {
  char ipstring[INET_ADDRSTRLEN] = {0};
  const char* ret = inet_ntop(AF_INET, &ip, ipstring, INET_ADDRSTRLEN);
  if (ret == NULL) return false;
  out = std::string(ipstring);
  return true;
}

// Only IPv4
bool get_interface_ip_in_subnet(uint32_t subnet_id, uint32_t subnet_mask, uint32_t &ip) {
  bool success = false;

#ifndef _WIN32
  // code adapted from
  struct ifaddrs * ifAddrStruct = NULL;
  getifaddrs(&ifAddrStruct);
  struct ifaddrs * firstifaddr = ifAddrStruct;
  ASSERT_NE(ifAddrStruct, NULL);
  while (ifAddrStruct != NULL) {
    if (ifAddrStruct->ifa_addr != NULL &&
        ifAddrStruct->ifa_addr->sa_family == AF_INET) {
      char* tmpAddrPtr = NULL;
      // check it is IP4 and not lo0.
      tmpAddrPtr = (char*)&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
      ASSERT_NE(tmpAddrPtr, NULL);
      if (tmpAddrPtr[0] != 127) {
        memcpy(&ip, tmpAddrPtr, 4);
        // test if it matches the subnet
        if ((ip & subnet_mask) == subnet_id) {
          success = true;
          break;
        }
      }
      //break;
    }
    ifAddrStruct=ifAddrStruct->ifa_next;
  }
  freeifaddrs(firstifaddr);

#else
  ULONG buf_size = 15000;
  ULONG ret;
  int iterations = 0;
  int max_tries = 3;
  IP_ADAPTER_ADDRESSES *addresses;

  do {
    std::cerr << "Allocating " << buf_size << " bytes for addresses" << std::endl;
    addresses = (IP_ADAPTER_ADDRESSES *)malloc(buf_size);
    if(addresses == NULL) {
      return success;
    }
    ret = GetAdaptersAddresses(AF_INET, 0, NULL, addresses, &buf_size);

    if(ret == ERROR_BUFFER_OVERFLOW) {
      free(addresses);
      addresses = NULL;
    }

    ++iterations;
  } while((ret == ERROR_BUFFER_OVERFLOW) && (iterations < max_tries));

  if(ret == NO_ERROR) {
    IP_ADAPTER_ADDRESSES *first_struct = addresses;
    IP_ADAPTER_ADDRESSES *cur_struct = addresses;
    for(; cur_struct != NULL; cur_struct = cur_struct->Next) {
      PIP_ADAPTER_UNICAST_ADDRESS first_addr = cur_struct->FirstUnicastAddress;
      PIP_ADAPTER_UNICAST_ADDRESS cur_addr = first_addr;

      // Some interface-level checking

      // Check if operational
      if(cur_struct->OperStatus != IfOperStatusUp) {
        std::cerr << "Not operational!" << std::endl;
        continue;
      }

      // Check if loopback
      std::cerr << "iftype: " << cur_struct->IfType << std::endl;
      if(cur_struct->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
        std::cerr << "loopback, skipping." << std::endl;
        continue;
      }

      // Check if a VM adapter.  Certain VM makers always use the same prefixes
      // on their MAC addresses.  This is the only way I can find to tell apart
      // a real IP from one that's just meant to talk to a VM on your machine.
      DWORD mac_addr_len = cur_struct->PhysicalAddressLength;

      if(mac_addr_len == 6) {
        // The first three bytes of known MAC addresses from different types of
        // VMs are the LAST three bytes in these integers.  First byte is all
        // zeroes and meaningless.
        unsigned known_vm_mac_prefixes[NUM_VM_MAC_ADDRS] = {
          0x00080027, // virtualbox
          0x00000569, // vmware
          0x00000c29, // vmware
          0x00005056, // vmware
          0x00001c42, // parallels
          0x000003ff, // microsoft virtual pc
          0x00000f4b, // virtual iron 4
          0x0000163e, // oracle vm, xen
        };

        // Get the MAC address into the same form as above.
        unsigned comparable_mac_addr = 0;
        comparable_mac_addr |= ((unsigned)cur_struct->PhysicalAddress[0] << 16);
        comparable_mac_addr |= ((unsigned)cur_struct->PhysicalAddress[1] << 8);
        comparable_mac_addr |= (unsigned)cur_struct->PhysicalAddress[2];

        bool match = false;
        for(int i = 0; i < NUM_VM_MAC_ADDRS; ++i) {
          if(comparable_mac_addr == known_vm_mac_prefixes[i]) {
            match = true;
            break;
          }
        }
        if(match) {
          continue;
        }
      } else {
        // This isn't a real MAC address, so this probably won't really send
        // packets.
        continue;
      }

      // Main goal: Iterate over each unicast address on this interface.  Use some criteria to pick what a good one is.
      //
      // To get the address:
      // 1. Get the Address field, which is a pointer to a SOCKET_ADDRESS structure
      // 2. Get the lpSockaddr field, which is a pointer to a sockaddr structure
      // 3. Cast struct sockaddr to sockaddr_in and get the sin_addr field
      std::cerr << "Found interface" << std::endl;
      while(cur_addr != NULL) {

        //
        //NOTE: The one, the one, don't go for the one.
        struct sockaddr_in *the_one =
          (struct sockaddr_in *)cur_addr->Address.lpSockaddr;
        char *tmp_addr_ptr = NULL;
        if(the_one != NULL) {
          tmp_addr_ptr = (char *)&the_one->sin_addr;
        }
        if(tmp_addr_ptr == NULL) {
          cur_addr = cur_addr->Next;
          continue;
        }
        std::string t;
        uint32_t ip_to_print;
        memcpy(&ip_to_print, tmp_addr_ptr, 4);
        ip_to_str(ip_to_print, t);
        std::cerr << "Found IP address: " << t << std::endl;

        memcpy(&ip, tmp_addr_ptr, 4);
        // Test if matches subnet
        if((ip & subnet_mask) == subnet_id) {
          std::cerr << "Match!" << std::endl;
          success = true;

          // We're done
          cur_struct = NULL;
          break;
        }

        cur_addr = cur_addr->Next;
      }
      if(cur_struct == NULL)
        break;
    }
  } else {
    std::cerr << "Error retrieving adapter addresses: " << ret << std::endl;
  }

  free(addresses);
#endif
  return success;
}

EXPORT std::string get_local_ip_as_str(bool print) {
  uint32_t ip = get_local_ip(print);
  if (ip == 0) return "127.0.0.1";
  else {
    std::string out;
    bool ip_conversion_success = ip_to_str(ip, out);
    ASSERT_TRUE(ip_conversion_success);
    return out;
  }
}

EXPORT uint32_t get_local_ip(bool print) {
  // see if TURI_SUBNET environment variable is set
  char* c_subnet_id = getenv("TURI_SUBNET_ID");
  char* c_subnet_mask = getenv("TURI_SUBNET_MASK");
  uint32_t subnet_id = 0;
  uint32_t subnet_mask = 0;
  std::string str_subnet_id, str_subnet_mask;
  // try to convert to a valid address when possible
  if (c_subnet_id != NULL) {
    if (!str_to_ip(c_subnet_id, subnet_id)) {
      std::cerr << "Unable to convert TURI_SUBNET_ID to a valid address. Cannot continue\n";
      exit(1);
    }
  }
  if (c_subnet_mask != NULL) {
    if (!str_to_ip(c_subnet_mask, subnet_mask)) {
      std::cerr << "Unable to convert TURI_SUBNET_MASK to a valid address. Cannot continue\n";
      exit(1);
    }
  }

  // error checking.
  // By the end of this block, we should either have both subnet_id and subnet_mask filled
  // to reasonable values, or are dead.

  if (c_subnet_id == NULL && c_subnet_mask != NULL) {
    // If subnet mask specified but not subnet ID, we cannot continue.
    std::cerr << "TURI_SUBNET_MASK specified, but TURI_SUBNET_ID not specified.\n";
    std::cerr << "We cannot continue\n";
    exit(1);
  }
  if (c_subnet_id != NULL && c_subnet_mask == NULL) {
    if (print) {
      std::cerr << "TURI_SUBNET_ID specified, but TURI_SUBNET_MASK not specified.\n";
      std::cerr << "We will try to guess a subnet mask\n";
    }
    // if subnet id specified, but not subnet mask. We can try to guess a mask
    // by finding the first "on" bit in the subnet id, and matching everything
    // to the left of it.
    // easiest way to do that is to left extend the subnet_id
    subnet_mask = subnet_id;
    subnet_mask = ntohl(subnet_mask);
    subnet_mask = subnet_mask | (subnet_mask << 1);
    subnet_mask = subnet_mask | (subnet_mask << 2);
    subnet_mask = subnet_mask | (subnet_mask << 4);
    subnet_mask = subnet_mask | (subnet_mask << 8);
    subnet_mask = subnet_mask | (subnet_mask << 16);
    subnet_mask = htonl(subnet_mask);
  }
  else {
    if (print) {
      std::cerr << "TURI_SUBNET_ID/TURI_SUBNET_MASK environment variables not defined.\n";
      std::cerr << "Using default values\n";
    }
  }
  ip_to_str(subnet_id, str_subnet_id);
  ip_to_str(subnet_mask, str_subnet_mask);

  uint32_t ip(0);
  bool success = get_interface_ip_in_subnet(subnet_id, subnet_mask, ip);

  // make sure this is a valid subnet address.
  if (print) {
      std::cerr << "Subnet ID: " << str_subnet_id << "\n";
      std::cerr << "Subnet Mask: " << str_subnet_mask << "\n";
      std::cerr << "Will find first IPv4 non-loopback address matching the subnet" << std::endl;
  }
  if (!success) {
    // if subnet addresses specified, and if we cannot find a valid network. Fail."
    if (c_subnet_id != NULL) {
      std::cerr << "Unable to find a network matching the requested subnet\n";
      exit(1);
    } else {
      std::cerr << "Unable to find any valid IPv4 address. Defaulting to loopback\n";
    }
  }
  return ip;
}

// TODO: This is broken on Windows, but is only used by RPC, so putting it off
EXPORT std::pair<size_t, int> get_free_tcp_port() {
#ifndef _WIN32
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  // uninteresting boiler plate. Set the port number and socket type
  sockaddr_in my_addr;
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = 0; // port 0.
  my_addr.sin_addr.s_addr = INADDR_ANY;
  memset(&(my_addr.sin_zero), '\0', 8);
  if (bind(sock, (sockaddr*)&my_addr, sizeof(my_addr)) < 0){
    logger(LOG_FATAL, "Failed to bind to a port 0! Unable to acquire a free TCP port!");
  }
  // get the sock information
  socklen_t slen;
  sockaddr addr;
  slen = sizeof(sockaddr);
  if (getsockname(sock, &addr, &slen) < 0) {
    logger(LOG_FATAL, "Failed to get port information about bound socket");
  }
  size_t freeport = ntohs(((sockaddr_in*)(&addr))->sin_port);
  std::pair<size_t, int> ret(freeport, sock);
  return ret;
#else
  log_and_throw("This function not supported on Windows");
#endif
}

} // namespace turi
