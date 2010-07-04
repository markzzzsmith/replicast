/*
 * aip_pton - convert address, interface and port strings into network
 * format.
 * 
 * formats supported -
 *
 * 	inet - e.g. 224.0.0.0%127.0.0.1:80
 *
 * 	inet6 - e.g. [ff02::1%eth0]:80
 *
 */

#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

#include "aip_pton.h"


int aip_pton_inet(const char *aip_str,
                   struct in_addr *addr,
                   struct in_addr *if_addr,
                   int *port,
		   enum aip_pton_errors *aip_pton_err)
{
	char *str = NULL;
	char *addr_str = NULL;
	char *s = NULL;
	char *if_addr_str = NULL;
	char *port_str = NULL;


	addr->s_addr = INADDR_NONE;
	if_addr->s_addr = INADDR_NONE;
	*port = 0;
	if (aip_pton_err != NULL) {
		*aip_pton_err = AIP_PTON_ERR_NO_ERROR;
	}

	str = strdup(aip_str);
	if (str == NULL) {
		return -1;
	}

	addr_str = str;

	s = strchr(str, '%');
	if (s != NULL) {
		if_addr_str = s + 1;
		*s = '\0';
	} else {
		if_addr_str = NULL;
	}

	s = strchr(str, ':');
	if (s != NULL) {
		port_str = s + 1;
		*s = '\0';
	} else {
		port_str = NULL;
	}

	if (inet_pton(AF_INET, addr_str, addr) != 1) {
		if (aip_pton_err != NULL) {
			*aip_pton_err = AIP_PTON_ERR_BAD_ADDR;
		}
		free(str);
		return -1;
	}

	if ((if_addr_str != NULL) &&
			(inet_pton(AF_INET, if_addr_str, if_addr) != 1)) {
		if (aip_pton_err != NULL) {
			*aip_pton_err = AIP_PTON_ERR_BAD_IF_ADDR;
		}
		free(str);
		return -1;
	}

	if (port_str != NULL) { 
		*port = atoi(port_str);
		if ((*port < 0) || (*port > 0xff)) {
			if (aip_pton_err != NULL) {
				*aip_pton_err = AIP_PTON_ERR_BAD_PORT;
			}
			*port = 0;
			free(str);
			return -1;
		}
	}

	free(str);

	return 0;

}

