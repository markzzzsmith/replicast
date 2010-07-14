/*
 * aip_ptoh - convert address, interface and port strings from presentation to
 * host native format.
 * 
 * formats supported -
 *
 * 	inet - e.g. 224.0.0.0%127.0.0.1:80
 *
 * 	inet6 - e.g. [ff02::1%eth0]:80
 *
 */

#include <stdio.h> /* remove after debugging */

#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

#include "aip_ptoh.h"


int aip_ptoh_inet(const char *aip_str,
                  struct in_addr *addr,
                  struct in_addr *if_addr,
                  unsigned int *port,
		  enum aip_ptoh_errors *aip_ptoh_err)
{
	char str[AIP_STR_INET_MAX_LEN + 1];
	char *addr_str = NULL;
	char *s = NULL;
	char *if_addr_str = NULL;
	char *port_str = NULL;


	addr->s_addr = INADDR_NONE;
	if_addr->s_addr = INADDR_NONE;
	*port = 0;
	if (aip_ptoh_err != NULL) {
		*aip_ptoh_err = AIP_PTOH_ERR_NO_ERROR;
	}

	strncpy(str, aip_str, AIP_STR_INET_MAX_LEN);
	str[AIP_STR_INET_MAX_LEN] = '\0';

	addr_str = str;

	s = strchr(str, '%');
	if (s != NULL) {
		*s = '\0';
		s++;
		if_addr_str = s;
	} else {
		s = str;
		if_addr_str = NULL;
	}

	s = strchr(s, ':');
	if (s != NULL) {
		*s = '\0';
		s++;
		port_str = s;
	} else {
		s = str;
		port_str = NULL;
	}

	if (inet_pton(AF_INET, addr_str, addr) != 1) {
		if (aip_ptoh_err != NULL) {
			*aip_ptoh_err = AIP_PTOH_ERR_BAD_ADDR;
		}
		return -1;
	}

	if ((if_addr_str != NULL) &&
			(inet_pton(AF_INET, if_addr_str, if_addr) != 1)) {
		if (aip_ptoh_err != NULL) {
			*aip_ptoh_err = AIP_PTOH_ERR_BAD_IF_ADDR;
		}
		return -1;
	}

	if (port_str != NULL) { 
		*port = atoi(port_str);
		if (*port > 0xffff) {
			if (aip_ptoh_err != NULL) {
				*aip_ptoh_err = AIP_PTOH_ERR_BAD_PORT;
			}
			*port = 0;
			return -1;
		}
	}

	return 0;

}



int aip_ptoh_inet6(const char *aip_str,
		   struct in6_addr *addr,
 		   char *if_str,
		   const unsigned int if_str_len,
		   unsigned int *port,
		   enum aip_ptoh_errors *aip_ptoh_err)
{
	char *str = NULL;
	char *addr_str = NULL;
	char *s = NULL;
	char *if_name_str = NULL;
	char *port_str = NULL;


	if (if_str_len == 0) {
		if (aip_ptoh_err != NULL) {
			*aip_ptoh_err = AIP_PTOH_ERR_IF_STR_LEN_BAD;
		}
		return -1;
	}

	inet_pton(AF_INET6, "::", addr);
	if_str[0] = '\0';
	*port = 0;
	if (aip_ptoh_err != NULL) {
		*aip_ptoh_err = AIP_PTOH_ERR_NO_ERROR;
	}

	str = strdup(aip_str);
	if (str == NULL) {
		if (aip_ptoh_err != NULL) {
			*aip_ptoh_err = AIP_PTOH_ERR_STRDUP;
		}
		return -1;
	}

	if (str[0] != '[') {
		if (aip_ptoh_err != NULL) {
			*aip_ptoh_err = AIP_PTOH_ERR_BAD_ADDR;
		}
		return -1;
	}

	addr_str = &str[1];

	s = strchr(str, '%');
	if (s != NULL) {
		*s = '\0';
		s++;
		if_name_str = s;
	} else {
		s = str;
		if_name_str = NULL;
	}

	s = strchr(s, ']');
	if (s != NULL) {
		*s = '\0';
		s++;
		if (*s != ':') {
			if (aip_ptoh_err != NULL) {
				*aip_ptoh_err = AIP_PTOH_ERR_BAD_PORT;
			}
			return -1;
		}	
		port_str = s + 1;
	} else {
		s = str;
		port_str = NULL;
	}

	if (inet_pton(AF_INET6, addr_str, addr) != 1) {
		if (aip_ptoh_err != NULL) {
			*aip_ptoh_err = AIP_PTOH_ERR_BAD_ADDR;
		}
		free(str);
		return -1;
	}

	if (if_name_str != NULL) {
		strncpy(if_str, if_name_str, if_str_len);
		if_str[if_str_len - 1] = '\0';
	}

	if (port_str != NULL) { 
		*port = atoi(port_str);
		if (*port > 0xffff) {
			if (aip_ptoh_err != NULL) {
				*aip_ptoh_err = AIP_PTOH_ERR_BAD_PORT;
			}
			*port = 0;
			free(str);
			return -1;
		}
	}

	free(str);

	return 0;

}

