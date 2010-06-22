/*
 * replicast - replicate an incoming multicast stream to multiple destination
 *	       multicast streams
 *
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "global.h"


struct inet_rx_mc_sock_params {
	struct in_addr mc_group;
	unsigned int port;
	struct in_addr in_intf_addr;	
};

struct inet_tx_mc_sock_params {
	int mc_ttl;
	int mc_loop;
	struct in_addr out_intf_addr;
	struct sockaddr_in *mc_dests;
	int mc_dests_num;	
};


enum {
	PKT_BUF_SIZE = 0xffff,
};


void close_sockets(void);

void inet_to_inet_mcast(int *inet_in_sock_fd,
			struct inet_rx_mc_sock_params rx_sock_parms,
			int *inet_out_sock_fd,
			struct inet_tx_mc_sock_params tx_sock_parms);

void exit_errno(const char *func_name, const unsigned int linenum, int errnum);

int open_inet_rx_mc_sock(const struct in_addr mc_group,
			 const unsigned int port,
			 const struct in_addr in_intf_addr);

void close_inet_rx_mc_sock(const int sock_fd);

int open_inet6_rx_mc_sock(const struct in6_addr mc_group,
			  const unsigned int port,
			  const unsigned int in_intf_idx);

void close_inet6_rx_mc_sock(const int sock_fd);

int open_inet_tx_mc_sock(const int mc_ttl,
			 const int mc_loop,
			 const struct in_addr out_intf_addr);

void close_inet_tx_mc_sock(int sock_fd);

int open_inet6_tx_mc_sock(const int mc_hops,
			  const int mc_loop,
			  const unsigned int out_intf_idx);

void close_inet6_tx_mc_sock(int sock_fd);

int inet_tx_mcast(const int sock_fd,
		  const void *pkt,
		  const size_t pkt_len,
 		  const struct sockaddr_in inet_mc_dests[],
		  const unsigned int mc_dests_num);

int inet6_tx_mcast(const int sock_fd,
		   const void *pkt,
		   const size_t pkt_len,
 		   const struct sockaddr_in6 inet6_mc_dests[],
		   const unsigned int mc_dests_num);

int inet_in_sock_fd = -1;
int inet6_in_sock_fd = -1;
int inet_out_sock_fd = -1;
int inet6_out_sock_fd = -1;

int main(int argc, char *argv[])
{
	struct inet_rx_mc_sock_params rx_sock_parms;
	struct inet_tx_mc_sock_params tx_sock_parms;
	struct sockaddr_in inet_mc_dests[3];
	

	atexit(close_sockets);

	inet_pton(AF_INET, "224.4.4.4", &rx_sock_parms.mc_group);
	rx_sock_parms.port = htons(1234);
	inet_pton(AF_INET, "1.1.1.1", &rx_sock_parms.in_intf_addr);

	tx_sock_parms.mc_ttl = 1;
	tx_sock_parms.mc_loop = 0;
	inet_pton(AF_INET, "1.1.1.1", &tx_sock_parms.out_intf_addr);

	memset(&inet_mc_dests[0], 0, sizeof(inet_mc_dests[0]));
	inet_mc_dests[0].sin_family = AF_INET;
	inet_pton(AF_INET, "224.5.5.5", &inet_mc_dests[0].sin_addr);
	inet_mc_dests[0].sin_port = htons(1234);
	
	memset(&inet_mc_dests[1], 0, sizeof(inet_mc_dests[1]));
	inet_mc_dests[1].sin_family = AF_INET;
	inet_pton(AF_INET, "224.6.6.6", &inet_mc_dests[1].sin_addr);
	inet_mc_dests[1].sin_port = htons(1234);
	
	memset(&inet_mc_dests[2], 0, sizeof(inet_mc_dests[2]));
	inet_mc_dests[2].sin_family = AF_INET;
	inet_pton(AF_INET, "224.7.7.7", &inet_mc_dests[2].sin_addr);
	inet_mc_dests[2].sin_port = htons(1234);

	tx_sock_parms.mc_dests = inet_mc_dests;
	tx_sock_parms.mc_dests_num = 3;
	
	inet_to_inet_mcast(&inet_in_sock_fd, rx_sock_parms, &inet_out_sock_fd,
		tx_sock_parms);

	return 0;

}


void close_sockets(void)
{


	close_inet_rx_mc_sock(inet_in_sock_fd);
	close_inet6_rx_mc_sock(inet6_in_sock_fd);
	close_inet_tx_mc_sock(inet_out_sock_fd);
	close_inet6_tx_mc_sock(inet6_out_sock_fd);

}


void inet_to_inet_mcast(int *inet_in_sock_fd,
			struct inet_rx_mc_sock_params rx_sock_parms,
			int *inet_out_sock_fd,
			struct inet_tx_mc_sock_params tx_sock_parms)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;


	*inet_in_sock_fd = open_inet_rx_mc_sock(rx_sock_parms.mc_group,
		rx_sock_parms.port, rx_sock_parms.in_intf_addr);
	if (*inet_in_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet_out_sock_fd = open_inet_tx_mc_sock(tx_sock_parms.mc_ttl,
		tx_sock_parms.mc_loop, tx_sock_parms.out_intf_addr);
	if (*inet_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	for ( ;; ) {
		rx_pkt_len = recv(*inet_in_sock_fd, pkt_buf, PKT_BUF_SIZE, 0);
		if (rx_pkt_len > 0) {
			inet_tx_mcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms.mc_dests,
				tx_sock_parms.mc_dests_num);
		} else if (rx_pkt_len == -1) {
			exit_errno(__func__, __LINE__, errno);
		}
	}


}


void exit_errno(const char *func_name, const unsigned int linenum, int errnum)
{


	fprintf(stderr, "%s():%d error: %s\n", func_name, linenum,
		strerror(errnum));
	exit(EXIT_FAILURE);

}


int open_inet_rx_mc_sock(const struct in_addr mc_group,
			 const unsigned int port,
			 const struct in_addr in_intf_addr)
{
	int ret;
	int sock_fd;
	struct sockaddr_in sa_in_mcaddr;
	struct ip_mreq ip_mcast_req;


	/* socket() */
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		return -1;
	}

	/* bind() to UDP port */
	memset(&sa_in_mcaddr, 0, sizeof(sa_in_mcaddr));
	sa_in_mcaddr.sin_family = AF_INET;
	sa_in_mcaddr.sin_addr =  mc_group;
	sa_in_mcaddr.sin_port = htons(port);
	ret = bind(sock_fd, (struct sockaddr *) &sa_in_mcaddr,
		sizeof(sa_in_mcaddr));
	if (ret == -1) {
		return -1;
	}

	/* setsockopt() to join mcast group */
	ip_mcast_req.imr_multiaddr = mc_group;
	ip_mcast_req.imr_interface = in_intf_addr;
	ret = setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		&ip_mcast_req, sizeof(ip_mcast_req));
	if (ret == -1) {
		return -1;
	}

	return sock_fd;

}


void close_inet_rx_mc_sock(const int sock_fd)
{


	if (sock_fd != -1) {
		close(sock_fd);
	}

}


int open_inet6_rx_mc_sock(const struct in6_addr mc_group,
			  const unsigned int port,
			  const unsigned int in_intf_idx)
{
	int ret;
	int sock_fd;
	struct sockaddr_in6 sa_in6_mcaddr;
	struct ipv6_mreq ipv6_mcast_req;


	/* socket() */
	sock_fd = socket(AF_INET6, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		return -1;
	}

	/* bind() to UDP port*/
	memset(&sa_in6_mcaddr, 0, sizeof(sa_in6_mcaddr));
	sa_in6_mcaddr.sin6_family = AF_INET6;
	sa_in6_mcaddr.sin6_addr = mc_group;
	sa_in6_mcaddr.sin6_port = htons(port);
	ret = bind(sock_fd, (struct sockaddr *) &sa_in6_mcaddr,
		sizeof(sa_in6_mcaddr));
	if (ret == -1) {
		return -1;
	}

	/* setsockopt() to join mcast group */
	ipv6_mcast_req.ipv6mr_multiaddr = mc_group;
	ipv6_mcast_req.ipv6mr_interface = in_intf_idx;
	ret = setsockopt(sock_fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
		&ipv6_mcast_req, sizeof(ipv6_mcast_req));
	if (ret == -1) {
		return -1;
	}

	return sock_fd;

}


void close_inet6_rx_mc_sock(const int sock_fd)
{


	if (sock_fd != -1) {
		close(sock_fd);
	}

}


int open_inet_tx_mc_sock(const int mc_ttl,
			 const int mc_loop,
			 const struct in_addr out_intf_addr)
{
	int sock_fd;
	uint8_t ttl;
	uint8_t mcloop;
	int ret;


	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		return -1;
	}

	if (mc_ttl > 0) {
		ttl = mc_ttl & 0xff;
		ret = setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_TTL,
			&ttl, sizeof(ttl));	
		if (ret == -1) {
			return -1;
		}
	}

	if (mc_loop == 1) {
		mcloop = 1;
	} else {
		mcloop = 0;
	}
	ret = setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP,
		&mc_loop, sizeof(mc_loop));
	if (ret == -1) {
		return -1;
	}

	ret = setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_IF, &out_intf_addr,
		sizeof(out_intf_addr));
	if (ret == -1) {
		return -1;
	}

	return sock_fd;

}


void close_inet_tx_mc_sock(int sock_fd)
{


	if (sock_fd != -1) {
		close(sock_fd);
	}

}


int open_inet6_tx_mc_sock(const int mc_hops,
			  const int mc_loop,
			  const unsigned int out_intf_idx)
{
	int sock_fd;
	uint8_t hops;
	uint8_t mcloop;
	uint16_t out_intf;
	int ret;
	


	sock_fd = socket(AF_INET6, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		return -1;
	}

	if (mc_hops > 0) {
		hops = mc_hops & 0xff;
		ret = setsockopt(sock_fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
			&hops, sizeof(hops));	
		if (ret == -1) {
			return -1;
		}
	}

	if (mc_loop == 1) {
		mcloop = 1;
	} else {
		mcloop = 0;
	}
	ret = setsockopt(sock_fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
		&mc_loop, sizeof(mc_loop));
	if (ret == -1) {
		return -1;
	}

	out_intf = out_intf_idx;
	ret = setsockopt(sock_fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &out_intf,
		sizeof(out_intf));
	if (ret == -1) {
		return -1;
	}

	return sock_fd;
			
}


void close_inet6_tx_mc_sock(int sock_fd)
{


	if (sock_fd != -1) {
		close(sock_fd);
	}

}


int inet_tx_mcast(const int sock_fd,
		  const void *pkt,
		  const size_t pkt_len,
 		  const struct sockaddr_in inet_mc_dests[],
		  const unsigned int mc_dests_num)
{
	unsigned int i;
	unsigned int tx_success = 0;
	ssize_t ret;


	for (i = 0; i < mc_dests_num; i++) {
		ret = sendto(sock_fd, pkt, pkt_len, 0, 	
			(struct sockaddr *)&inet_mc_dests[i],
			sizeof(struct sockaddr_in));
		if (ret != -1) {
			tx_success++;
		}
	}

	return tx_success;

}


int inet6_tx_mcast(const int sock_fd,
		   const void *pkt,
		   const size_t pkt_len,
 		   const struct sockaddr_in6 inet6_mc_dests[],
		   const unsigned int mc_dests_num)
{
	unsigned int i;
	unsigned int tx_success = 0;
	ssize_t ret;


	for (i = 0; i < mc_dests_num; i++) {
		ret = sendto(sock_fd, pkt, pkt_len, 0, 	
			(struct sockaddr *)&inet6_mc_dests[i],
			sizeof(struct sockaddr_in6));
		if (ret != -1) {
			tx_success++;
		}
	}

	return tx_success;

}
