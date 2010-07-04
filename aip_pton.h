#ifndef __AIP_PTON_H
#define __AIP_PTON_H


#include <netinet/in.h>

enum aip_pton_errors {
	AIP_PTON_ERR_NO_ERROR,
	AIP_PTON_ERR_BAD_ADDR,
	AIP_PTON_ERR_BAD_IF_ADDR,
	AIP_PTON_ERR_BAD_PORT
};


int aip_pton_inet(const char *aip_str,
		   struct in_addr *addr,
		   struct in_addr *if_addr,
		   int *port,
		   enum aip_pton_errors *aip_pton_err);


#endif /* __AIP_PTON_H */
