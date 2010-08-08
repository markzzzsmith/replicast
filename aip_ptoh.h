/*
 * aip_ptoh - convert address, interface and port strings from presentation to
 * host native format.
 * 
 * formats supported -
 *
 *	inet - e.g. 224.0.0.0%127.0.0.1:80
 *
 *	inet6 - e.g. [ff02::1%eth0]:80
 *
 */
#ifndef __AIP_PTOH_H
#define __AIP_PTOH_H


#include <net/if.h>
#include <netinet/in.h>

enum {
	/* <inet addr>%<ifaddr>:<port> */
	AIP_STR_INET_MAX_LEN = INET_ADDRSTRLEN + 1 + INET_ADDRSTRLEN + 1 + 5,

	/* [<inet6 addr>%<ifname>]:<port> */
	AIP_STR_INET6_MAX_LEN = 1 + INET6_ADDRSTRLEN + 1 + IF_NAMESIZE + 1\
		+ 1 + 5,
};

enum aip_ptoh_errors {
	AIP_PTOH_ERR_NO_ERROR,
	AIP_PTOH_ERR_BAD_ADDR,
	AIP_PTOH_ERR_BAD_IF_ADDR,
	AIP_PTOH_ERR_BAD_PORT,
};


int aip_ptoh_inet(const char *aip_str,
		  struct in_addr *addr,
		  struct in_addr *if_addr,
		  unsigned int *port,
		  enum aip_ptoh_errors *aip_ptoh_err);

int ap_pton_inet_csv(const char *ap_inet_csv_str,
		     struct sockaddr_in **ap_sa_list,
		     const int max_sa_list_len,
		     char *ap_err_str,
		     const unsigned int ap_err_str_size);

int aip_ptoh_inet6(const char *aip_str,
		   struct in6_addr *addr,
		   unsigned int *ifidx,
		   unsigned int *port,
		   enum aip_ptoh_errors *aip_ptoh_err);


#endif /* __AIP_PTOH_H */
