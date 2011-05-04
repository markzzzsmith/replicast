/*
 * aip_ptox - convert address, interface and port strings from presentation to
 * different formats e.g. host or network
 * 
 * formats supported -
 *
 *	inet - e.g. 224.0.0.0%127.0.0.1:80
 *
 *	inet6 - e.g. [ff02::1%eth0]:80
 *
 * Copyright (C) 2011 Mark Smith <markzzzsmith@yahoo.com.au>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA. 
 */
#ifndef __INETADDR_H
#define __INETADDR_H


#include <net/if.h>
#include <netinet/in.h>

enum {
	/* <inet addr>%<ifaddr>:<port> */
	AIP_STR_INET_MAX_LEN = INET_ADDRSTRLEN + 1 + INET_ADDRSTRLEN + 1 + 5,

	/* [<inet6 addr>%<ifname>]:<port> */
	AIP_STR_INET6_MAX_LEN = 1 + INET6_ADDRSTRLEN + 1 + IF_NAMESIZE + 1\
		+ 1 + 5,
};

enum inetaddr_errors {
	INETADDR_ERR_NONE,
	INETADDR_ERR_ADDR,
	INETADDR_ERR_IFADDR,
	INETADDR_ERR_PORT,
};

int aip_ptoh_inet(const char *aip_str,
		  struct in_addr *addr,
		  struct in_addr *if_addr,
		  unsigned int *port,
		  enum inetaddr_errors *aip_ptoh_err);

void aip_htop_inet(const struct in_addr *addr,
		   const struct in_addr *if_addr,
		   const unsigned int port,
		   char *aip_str,
		   const unsigned int aip_str_size);

void ap_htop_inet(const struct in_addr *addr,
		  const unsigned int port,
		  char *ap_str,
		  const unsigned int ap_str_size);


int ap_pton_inet_csv(const char *ap_csv_str,
		     struct sockaddr_in *sa_list,
		     const int max_sa_list_entries,
		     const unsigned char ignore_errors,
		     const unsigned char sentinel,
		     enum inetaddr_errors *ap_err,
		     char *ap_err_str,
		     const unsigned int ap_err_str_size);

int aip_ptoh_inet6(const char *aip_str,
		   struct in6_addr *addr,
		   unsigned int *ifidx,
		   unsigned int *port,
		   enum inetaddr_errors *aip_ptoh_err);

void aip_htop_inet6(const struct in6_addr *addr,
		    const unsigned int ifidx,
		    const unsigned int port,
		    char *aip_str,
		    const unsigned int aip_str_size);

void ap_htop_inet6(const struct in6_addr *addr,
                   const unsigned int port,
                   char *ap_str,
                   const unsigned int ap_str_size);

int ap_pton_inet6_csv(const char *ap_csv_str,
		      struct sockaddr_in6 *sa_list,
		      const int max_sa_list_entries,
		      const unsigned char ignore_errors,
		      const unsigned char sentinel,
		      enum inetaddr_errors *ap_err,
		      char *ap_err_str,
		      const unsigned int ap_err_str_size);


int inet_if_addr(const char *str,
		 struct in_addr *if_addr);


unsigned int num_inet_mcaddrs(const struct sockaddr_in *sa_list,
			      const int sa_list_len);

unsigned int num_inet6_mcaddrs(const struct sockaddr_in6 *sa_list,
			       const int sa_list_len);

#endif /* __INETADDR_H */
