/*
 * inetaddr - convert address, interface and port strings from presentation to
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
#include <unistd.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "inetaddr.h"


static int get_if_addr(const char *ifname, struct in_addr *addr);


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
		*aip_ptoh_err = AIP_PTOX_ERR_NO_ERROR;
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

	s = strrchr(s, ':');
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
			*aip_ptoh_err = AIP_PTOX_ERR_BAD_ADDR;
		}
		return -1;
	}

	if ((if_addr_str != NULL) &&
				 (inet_if_addr(if_addr_str, if_addr) == -1)) {
		if (aip_ptoh_err != NULL) {
			*aip_ptoh_err = AIP_PTOX_ERR_BAD_IF_ADDR;
		}
		return -1;
	}

	if (port_str != NULL) { 
		*port = atoi(port_str);
		if (*port > 0xffff) {
			if (aip_ptoh_err != NULL) {
				*aip_ptoh_err = AIP_PTOX_ERR_BAD_PORT;
			}
			*port = 0;
			return -1;
		}
	}

	return 0;

}


int ap_pton_inet_csv(const char *ap_inet_csv_str,
		     struct sockaddr_in **ap_sa_list,
		     const int max_sa_list_len,
		     char *ap_err_str,
		     const unsigned int ap_err_str_size)
{
	int sa_list_len = 0;
	char aip_str[AIP_STR_INET_MAX_LEN + 1];
	int more_aip_str = 0;
	const char *curr_aip_str = NULL;
	char *ap_csv_comma_ptr = NULL;
	char *aip_str_comma_ptr = NULL;
	struct sockaddr_in ap_sa;
	struct in_addr tmp_in_addr;
	unsigned int port;
	int ret;
	enum aip_ptoh_errors aip_ptoh_err;


	if (*ap_sa_list != NULL) {
		return -1;
	}

	curr_aip_str = ap_inet_csv_str;

	strncpy(aip_str, curr_aip_str, AIP_STR_INET_MAX_LEN);
	aip_str[AIP_STR_INET_MAX_LEN] = '\0';

	aip_str_comma_ptr = strchr(aip_str, ',');
	if (aip_str_comma_ptr != NULL) {
		*aip_str_comma_ptr = '\0';
	}

	do {
		memset(&ap_sa, 0, sizeof(ap_sa));

		ret = aip_ptoh_inet(aip_str, &ap_sa.sin_addr, &tmp_in_addr,
				    &port, &aip_ptoh_err);

		if (ret != -1) {
			ap_sa.sin_family = AF_INET;
			ap_sa.sin_port = htons(port);
			sa_list_len++;

			*ap_sa_list = realloc(*ap_sa_list,
					      sa_list_len * sizeof(ap_sa));
			memcpy(&((*ap_sa_list)[sa_list_len - 1]), &ap_sa,
			       sizeof(ap_sa));
		} else {
			strncpy(ap_err_str, aip_str, ap_err_str_size - 1);
			ap_err_str[ap_err_str_size] = '\0';
			return -1;
		}

		ap_csv_comma_ptr = strchr(curr_aip_str, ',');
		if (ap_csv_comma_ptr == NULL) {
			more_aip_str = 0;
		} else {
			curr_aip_str = ap_csv_comma_ptr + 1;

			strncpy(aip_str, curr_aip_str, AIP_STR_INET_MAX_LEN);
			aip_str[AIP_STR_INET_MAX_LEN] = '\0';

			aip_str_comma_ptr = strchr(aip_str, ',');
			if (aip_str_comma_ptr != NULL) {
				*aip_str_comma_ptr = '\0';
			}
			more_aip_str = 1;
		}

	} while (more_aip_str &&
		((sa_list_len < max_sa_list_len ) || (max_sa_list_len == 0)));

	return sa_list_len;

}


int aip_ptoh_inet6(const char *aip_str,
		   struct in6_addr *addr,
		   unsigned int *ifidx,
		   unsigned int *port,
		   enum aip_ptoh_errors *aip_ptoh_err)
{
	char str[AIP_STR_INET6_MAX_LEN + 1];
	char *addr_str = NULL;
	char *s = NULL;
	char *if_name_str = NULL;
	char *port_str = NULL;


	inet_pton(AF_INET6, "::", addr);
	*ifidx = 0;
	*port = 0;
	if (aip_ptoh_err != NULL) {
		*aip_ptoh_err = AIP_PTOX_ERR_NO_ERROR;
	}

	strncpy(str, aip_str, AIP_STR_INET6_MAX_LEN);
	str[AIP_STR_INET6_MAX_LEN] = '\0';

	if (str[0] != '[') {
		if (aip_ptoh_err != NULL) {
			*aip_ptoh_err = AIP_PTOX_ERR_BAD_ADDR;
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
				*aip_ptoh_err = AIP_PTOX_ERR_BAD_PORT;
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
			*aip_ptoh_err = AIP_PTOX_ERR_BAD_ADDR;
		}
		return -1;
	}

	if (if_name_str != NULL) {
		*ifidx = if_nametoindex(if_name_str);
	}

	if (port_str != NULL) { 
		*port = atoi(port_str);
		if (*port > 0xffff) {
			if (aip_ptoh_err != NULL) {
				*aip_ptoh_err = AIP_PTOX_ERR_BAD_PORT;
			}
			*port = 0;
			return -1;
		}
	}

	return 0;

}


int inet_if_addr(const char *str,
                 struct in_addr *if_addr)
{
	int ret;


	ret = inet_pton(AF_INET, str, if_addr);

	if (ret == 1) {
		return 0;
	}

	ret = get_if_addr(str, if_addr);

	if (ret == 0) {
		return 0;
	} else {
		return -1;
	}

}


static int get_if_addr(const char *ifname, struct in_addr *addr)
{
	int sock_fd;
	struct ifreq ifr;
	int ioctl_ret;
	struct sockaddr_in *sa_in = NULL;


	addr->s_addr = INADDR_NONE;

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ-1] = '\0';	

	ioctl_ret = ioctl(sock_fd, SIOCGIFADDR, &ifr);

	if (close(sock_fd) == -1) {
		return -1;
	}

	if (ioctl_ret == -1) {
		return -1;
	}

	sa_in = (struct sockaddr_in *)&ifr.ifr_addr;

	memcpy(&addr->s_addr, &sa_in->sin_addr, sizeof(in_addr_t));

	return 0;

}
