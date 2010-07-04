#ifndef __AIP_PTON_H
#define __AIP_PTON_H


#include <netinet/in.h>

enum aip_pton_errors {
	AIP_PTON_ERR_NO_ERROR,
	AIP_PTON_ERR_STRDUP,
	AIP_PTON_ERR_BAD_ADDR,
	AIP_PTON_ERR_BAD_IF_ADDR,
	AIP_PTON_ERR_BAD_PORT,
	AIP_PTON_ERR_IF_STR_LEN_BAD,
};


int aip_pton_inet(const char *aip_str,
		  struct in_addr *addr,
		  struct in_addr *if_addr,
		  unsigned int *port,
		  enum aip_pton_errors *aip_pton_err);

int aip_pton_inet6(const char *aip_str,
		   struct in6_addr *addr,
		   char *if_str,
		   const unsigned int if_str_len,
		   unsigned int *port,
		   enum aip_pton_errors *aip_pton_err);


#endif /* __AIP_PTON_H */
