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


#include <netinet/in.h>

enum {
	AIP_STR_INET_MAX_LEN = 37,  /* XXX.XXX.XXX.XXX%XXX.XXX.XXX.XXX:65432 */
};

enum aip_ptoh_errors {
	AIP_PTOH_ERR_NO_ERROR,
	AIP_PTOH_ERR_STRDUP,
	AIP_PTOH_ERR_BAD_ADDR,
	AIP_PTOH_ERR_BAD_IF_ADDR,
	AIP_PTOH_ERR_BAD_PORT,
	AIP_PTOH_ERR_IF_STR_LEN_BAD,
};


int aip_ptoh_inet(const char *aip_str,
		  struct in_addr *addr,
		  struct in_addr *if_addr,
		  unsigned int *port,
		  enum aip_ptoh_errors *aip_ptoh_err);

int aip_ptoh_inet6(const char *aip_str,
		   struct in6_addr *addr,
		   char *if_str,
		   const unsigned int if_str_len,
		   unsigned int *port,
		   enum aip_ptoh_errors *aip_ptoh_err);


#endif /* __AIP_PTOH_H */
