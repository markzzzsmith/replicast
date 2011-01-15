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

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "inetaddr.h"

#include "stringz.h"


static int get_if_addr(const char *ifname, struct in_addr *addr);

static void zero_comma(char *str);


int aip_ptoh_inet(const char *aip_str,
                  struct in_addr *addr,
                  struct in_addr *if_addr,
                  unsigned int *port,
		  enum inetaddr_errors *aip_ptoh_err)
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
		*aip_ptoh_err = INETADDR_ERR_NONE;
	}

	strnzcpy(str, aip_str, AIP_STR_INET_MAX_LEN + 1);

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
			*aip_ptoh_err = INETADDR_ERR_ADDR;
		}
		return -1;
	}

	if ((if_addr_str != NULL) &&
				 (inet_if_addr(if_addr_str, if_addr) == -1)) {
		if (aip_ptoh_err != NULL) {
			*aip_ptoh_err = INETADDR_ERR_IFADDR;
		}
		return -1;
	}

	if (port_str != NULL) { 
		*port = atoi(port_str);
		if (*port > 0xffff) {
			if (aip_ptoh_err != NULL) {
				*aip_ptoh_err = INETADDR_ERR_PORT;
			}
			*port = 0;
			return -1;
		}
	}

	return 0;

}


void aip_htop_inet(const struct in_addr *addr,
		   const struct in_addr *if_addr,
		   const unsigned int port,
		   char *aip_str,
		   const unsigned int aip_str_size)
{
	const unsigned int aip_tmp_str_size = AIP_STR_INET_MAX_LEN + 1;
	char aip_tmp_str[AIP_STR_INET_MAX_LEN + 1] = { 0 };
	unsigned int aip_tmp_str_bytes_left = aip_tmp_str_size;
	char *s;


	if (aip_str_size < aip_tmp_str_size ) {
		if (aip_str_size) {
			aip_str[0] = '\0';
		}
		return;
	}

	s = aip_tmp_str;

	if (inet_ntop(AF_INET, addr, s, aip_tmp_str_bytes_left) == NULL) {
		aip_str[0] = '\0';
		return;
	}

	aip_tmp_str_bytes_left -= strlen(s) + 1;
	
	s = strchr(s, '\0');

	if (if_addr->s_addr != INADDR_ANY) {
		*s = '%';
		s++;
		aip_tmp_str_bytes_left--;

		if (inet_ntop(AF_INET, if_addr, s, aip_tmp_str_bytes_left) ==
									NULL) {
			aip_str[0] = '\0';
			return;
		}
	
		aip_tmp_str_bytes_left -= strlen(s) + 1;

		s = strchr(s, '\0');
	}
	
	*s = ':';
	s++;
	aip_tmp_str_bytes_left--;

	snprintf(s, aip_tmp_str_bytes_left, "%d", port);
	
	strnzcpy(aip_str, aip_tmp_str, aip_str_size);

}


void ap_htop_inet(const struct in_addr *addr,
		  const unsigned int port,
		  char *ap_str,
		  const unsigned int ap_str_size)
{
	const unsigned int ap_str_min_size = INET_ADDRSTRLEN + 1 + 5 + 1;
	const unsigned int aip_tmp_str_size = AIP_STR_INET_MAX_LEN + 1;
	char aip_tmp_str[aip_tmp_str_size];
	struct in_addr empty_ifaddr;


	if (ap_str_size < ap_str_min_size) {
		if (ap_str_size) {
			ap_str[0] = '\0';
		}
		return;
	}

	empty_ifaddr.s_addr = INADDR_ANY;

	aip_htop_inet(addr, &empty_ifaddr, port, aip_tmp_str, aip_tmp_str_size);

	strnzcpy(ap_str, aip_tmp_str, ap_str_size);

}


int ap_pton_inet_csv(const char *ap_csv_str,
		     struct sockaddr_in *sa_list,
		     const int max_sa_list_entries,
		     const unsigned char ignore_errors,
		     const unsigned char sentinel,
		     enum inetaddr_errors *ap_err,
		     char *ap_err_str,
		     const unsigned int ap_err_str_size)
{
	const char *curr_aip_str = NULL;
	char aip_str[AIP_STR_INET_MAX_LEN + 1];
	char *curr_aip_str_comma = NULL;
	struct sockaddr_in ap_sa;
	int ret;
	struct in_addr tmp_in_addr;
	unsigned int port;
	enum inetaddr_errors aip_ptoh_err;
	int sa_list_len = 0;
	unsigned char more_aip_str;


	curr_aip_str = ap_csv_str;

	strnzcpy(aip_str, curr_aip_str, AIP_STR_INET_MAX_LEN + 1);
	zero_comma(aip_str);

	do {
		memset(&ap_sa, 0, sizeof(ap_sa));

		ret = aip_ptoh_inet(aip_str, &ap_sa.sin_addr, &tmp_in_addr,
				    &port, &aip_ptoh_err);
		if (ret != -1) {
			sa_list_len++;

			if (sa_list != NULL) {
				ap_sa.sin_family = AF_INET;
				ap_sa.sin_port = htons(port);
				sa_list[sa_list_len - 1] = ap_sa;
			}
			
		} else {
			if (!ignore_errors) {
				if (aip_ptoh_err != INETADDR_ERR_IFADDR) {
					if (ap_err_str != NULL &&
					    ap_err_str_size > 0) {
						strnzcpy(ap_err_str, aip_str,
							 ap_err_str_size);
					}
					if (ap_err != NULL) {
						*ap_err = aip_ptoh_err;
					}
					return -1;
				}
			}
		}

		curr_aip_str_comma = strchr(curr_aip_str, ',');
		if (curr_aip_str_comma == NULL) {
			more_aip_str = 0;
		} else {
			curr_aip_str = curr_aip_str_comma + 1;

			strnzcpy(aip_str, curr_aip_str,
						AIP_STR_INET_MAX_LEN + 1);
			zero_comma(aip_str);

			more_aip_str = 1;
		}
	} while (more_aip_str &&
		 sa_list_len < INT_MAX &&
		 ((sa_list_len < max_sa_list_entries) ||
		  max_sa_list_entries == 0));

	if (sentinel && sa_list != NULL) {
		memset(&ap_sa, 0, sizeof(ap_sa));
		ap_sa.sin_family = AF_UNSPEC;
		sa_list[sa_list_len] = ap_sa;
	}
	
	return sa_list_len;
}


int aip_ptoh_inet6(const char *aip_str,
		   struct in6_addr *addr,
		   unsigned int *ifidx,
		   unsigned int *port,
		   enum inetaddr_errors *aip_ptoh_err)
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
		*aip_ptoh_err = INETADDR_ERR_NONE;
	}

	strnzcpy(str, aip_str, AIP_STR_INET6_MAX_LEN + 1);

	if (str[0] != '[') {
		if (aip_ptoh_err != NULL) {
			*aip_ptoh_err = INETADDR_ERR_ADDR;
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
				*aip_ptoh_err = INETADDR_ERR_PORT;
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
			*aip_ptoh_err = INETADDR_ERR_ADDR;
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
				*aip_ptoh_err = INETADDR_ERR_PORT;
			}
			*port = 0;
			return -1;
		}
	}

	return 0;

}


void aip_htop_inet6(const struct in6_addr *addr,
                    const unsigned int ifidx,
                    const unsigned int port,
                    char *aip_str,
                    const unsigned int aip_str_size)
{
	const unsigned int aip_tmp_str_size = AIP_STR_INET6_MAX_LEN + 1;
	char aip_tmp_str[AIP_STR_INET6_MAX_LEN + 1] = { 0 };
	unsigned int aip_tmp_str_bytes_left = aip_tmp_str_size;
	char *s;
	char tmp_ifname[IFNAMSIZ];


	if (aip_str_size < aip_tmp_str_size) {
		if (aip_str_size) {
			aip_str[0] = '\0';
		}
		return;
	}

	s = aip_tmp_str;

	*s = '[';
	s++;
	aip_tmp_str_bytes_left--;

	if (inet_ntop(AF_INET6, addr, s, aip_tmp_str_bytes_left) == NULL) {
		aip_str[0] = '\0';
		return;
	}

	aip_tmp_str_bytes_left -= strlen(s) + 1;
	
	s = strchr(s, '\0');

	if ((ifidx != 0) && (if_indextoname(ifidx, tmp_ifname) != NULL)) {
		*s = '%';
		s++;
		aip_tmp_str_bytes_left--;

		strnzcpy(s, tmp_ifname, aip_tmp_str_bytes_left);

		aip_tmp_str_bytes_left -= strlen(s) + 1;
		s = strchr(s, '\0');
	}

	*s = ']';
	s++;
	aip_tmp_str_bytes_left--;
	
	*s = ':';
	s++;
	aip_tmp_str_bytes_left--;

	snprintf(s, aip_tmp_str_bytes_left, "%d", port);
	
	strnzcpy(aip_str, aip_tmp_str, aip_str_size);

}


void ap_htop_inet6(const struct in6_addr *addr,
                   const unsigned int port,
                   char *ap_str,
                   const unsigned int ap_str_size)
{
	const unsigned int ap_str_min_size =
					1 + INET6_ADDRSTRLEN + 1 + 1 + 5 + 1;
	const unsigned int aip_tmp_str_size = AIP_STR_INET6_MAX_LEN + 1;
	char aip_tmp_str[aip_tmp_str_size];


	if (ap_str_size < ap_str_min_size) {
		if (ap_str_size) {
			ap_str[0] = '\0';
		}
		return;
	}

	aip_htop_inet6(addr, 0, port, aip_tmp_str, aip_tmp_str_size);

	strnzcpy(ap_str, aip_tmp_str, ap_str_size);

}


int ap_pton_inet6_csv(const char *ap_csv_str,
		      struct sockaddr_in6 *sa_list,
		      const int max_sa_list_entries,
		      const unsigned char ignore_errors,
		      const unsigned char sentinel,
		      enum inetaddr_errors *ap_err,
		      char *ap_err_str,
		      const unsigned int ap_err_str_size)
{
	const char *curr_aip_str = NULL;
	char aip_str[AIP_STR_INET6_MAX_LEN + 1];
	char *curr_aip_str_comma = NULL;
	struct sockaddr_in6 ap_sa;
	int ret;
	unsigned int ifidx;
	unsigned int port;
	enum inetaddr_errors aip_ptoh_err;
	int sa_list_len = 0;
	unsigned char more_aip_str;


	curr_aip_str = ap_csv_str;

	strnzcpy(aip_str, curr_aip_str, AIP_STR_INET6_MAX_LEN + 1);
	zero_comma(aip_str);

	do {
		memset(&ap_sa, 0, sizeof(ap_sa));

		ret = aip_ptoh_inet6(aip_str, &ap_sa.sin6_addr, &ifidx,
				     &port, &aip_ptoh_err);
		if (ret != -1) {
			sa_list_len++;

			if (sa_list != NULL) {
				ap_sa.sin6_family = AF_INET6;
				ap_sa.sin6_port = htons(port);
				sa_list[sa_list_len - 1] = ap_sa;
			}
			
		} else {
			if (!ignore_errors) {
				if (ap_err_str != NULL &&
						 ap_err_str_size > 0) {
					strnzcpy(ap_err_str, aip_str,
							 ap_err_str_size);
				}
				if (ap_err != NULL) {
					*ap_err = aip_ptoh_err;
				}
				return -1;
			}
		}

		curr_aip_str_comma = strchr(curr_aip_str, ',');
		if (curr_aip_str_comma == NULL) {
			more_aip_str = 0;
		} else {
			curr_aip_str = curr_aip_str_comma + 1;

			strnzcpy(aip_str, curr_aip_str,
						AIP_STR_INET6_MAX_LEN + 1);
			zero_comma(aip_str);

			more_aip_str = 1;
		}
	} while (more_aip_str &&
		 sa_list_len < INT_MAX &&
		 ((sa_list_len < max_sa_list_entries) ||
		  max_sa_list_entries == 0));

	if (sentinel && sa_list != NULL) {
		memset(&ap_sa, 0, sizeof(ap_sa));
		ap_sa.sin6_family = AF_UNSPEC;
		sa_list[sa_list_len] = ap_sa;
	}
	
	return sa_list_len;
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


unsigned int num_inet_mcaddrs(const struct sockaddr_in *sa_list,
			      const int sa_list_len)
{
	int sa_num;
	unsigned int num_mcaddrs = 0;
	const struct sockaddr_in *sa;


	for (sa_num = 0; sa_num < sa_list_len; sa_num++) {
		sa = &sa_list[sa_num];
		if ( IN_MULTICAST( ntohl(sa->sin_addr.s_addr) ) ) {
			num_mcaddrs++;
		} 
	}

	return num_mcaddrs;

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

	strnzcpy(ifr.ifr_name, ifname, IFNAMSIZ);

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


static void zero_comma(char *str)
{
	char *comma_pos;


	comma_pos = strchr(str, ',');
	if (comma_pos != NULL) {
		*comma_pos = '\0';
	}

}



