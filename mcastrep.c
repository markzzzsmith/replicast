/*
 * mcastrep - replicate an incoming multicast stream to multiple destination
 *	      multicast streams
 *
 */

#include <stdio.h> /* remove after testing if necessary */
#include <stdlib.h> /* remove after testing if necessary */
#include <string.h>
#include <unistd.h> /* sleep() - remove after testing */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "global.h"


struct mcast_group {
	int addr_family;
	union {
		struct {
			struct in_addr group_addr;
			struct in_addr intf_addr;
		} inet_group;
		struct {
			struct in6_addr group_addr;
			unsigned int intf_index;
		} inet6_group;
	};
};


int open_mc_rx_socket(const struct mcast_group mc_group, unsigned int port);


int main(int argc, char *argv[])
{
	struct mcast_group mc_group;
	int sock_fd;

	mc_group.addr_family = AF_INET;
	mc_group.inet_group.group_addr.s_addr = htonl(INADDR_ALLRTRS_GROUP);
	mc_group.inet_group.intf_addr.s_addr = htonl(INADDR_ANY);

/*
	mc_group.addr_family = AF_INET6;
	inet_pton(AF_INET6, "ff05::5", &mc_group.inet6_group.group_addr);
	mc_group.inet6_group.intf_index = 115;
*/

	sock_fd = open_mc_rx_socket(mc_group, 1234);

	if (sock_fd == -1) {
		perror("");
	}

	sleep(20);	

	return 0;

}


int open_mc_rx_socket(const struct mcast_group mc_group, unsigned int port)
{
	int ret;
	int sock_fd;
	struct sockaddr_in sa_in_mcaddr;
	struct sockaddr_in6 sa_in6_mcaddr;
	struct ip_mreq ip_mcast_req;
	struct ipv6_mreq ipv6_mcast_req;


	switch (mc_group.addr_family) {
	case AF_INET:
		break;
	case AF_INET6:
		break;
	default:
		return -1;
	}
	
	/* socket() */
	sock_fd = socket(mc_group.addr_family, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		return -1;
	}

	/* bind() */
	switch (mc_group.addr_family) {
	case AF_INET:
		memset(&sa_in_mcaddr, 0, sizeof(sa_in_mcaddr));
		sa_in_mcaddr.sin_family = AF_INET;
		memcpy(&sa_in_mcaddr.sin_addr.s_addr,
			&mc_group.inet_group.group_addr,
			sizeof(sa_in_mcaddr.sin_addr.s_addr));
		sa_in_mcaddr.sin_port = htons(port);
		ret = bind(sock_fd, (struct sockaddr *) &sa_in_mcaddr,
			sizeof(sa_in_mcaddr));
		if (ret == -1) {
			return -1;
		}
		break;
	case AF_INET6:
		memset(&sa_in6_mcaddr, 0, sizeof(sa_in_mcaddr));
		sa_in6_mcaddr.sin6_family = AF_INET6;
		memcpy(&sa_in6_mcaddr.sin6_addr,
			&mc_group.inet6_group.group_addr,
			sizeof(sa_in6_mcaddr.sin6_addr));
		sa_in6_mcaddr.sin6_port = htons(port);
		ret = bind(sock_fd, (struct sockaddr *) &sa_in6_mcaddr,
			sizeof(sa_in6_mcaddr));
		if (ret == -1) {
			return -1;
		}
		break;
	default:
		return -1;
	}

	/*  join multicast group */
	switch (mc_group.addr_family) {
	case AF_INET:
		memcpy(&ip_mcast_req.imr_multiaddr,
			&mc_group.inet_group.group_addr,
			sizeof(ip_mcast_req.imr_multiaddr));
		ip_mcast_req.imr_interface.s_addr =
			mc_group.inet_group.intf_addr.s_addr;
		ret = setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			&ip_mcast_req, sizeof(ip_mcast_req));
		if (ret == -1) {
			return -1;
		}
		break;
	case AF_INET6:
		memcpy(&ipv6_mcast_req.ipv6mr_multiaddr,
			&mc_group.inet6_group.group_addr,
			sizeof(ipv6_mcast_req.ipv6mr_multiaddr));
		ipv6_mcast_req.ipv6mr_interface =
			mc_group.inet6_group.intf_index;
		ret = setsockopt(sock_fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
			&ipv6_mcast_req, sizeof(ipv6_mcast_req));
		if (ret == -1) {
			return -1;
		}
		break;
	default:
		return -1;;
	}

	return sock_fd;

}

