/*
 * replicast - replicate an incoming multicast stream to multiple destination
 *	       multicast streams
 *
 */

#include <stdint.h>
#include <stdio.h> /* remove after testing if necessary */
#include <stdlib.h> /* remove after testing if necessary */
#include <string.h>
#include <unistd.h> /* sleep() - remove after testing */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "global.h"


int open_inet_rx_mc_sock(const struct in_addr mc_group,
			 const unsigned int port,
			 const struct in_addr in_intf_addr);

void close_inet_rx_mc_sock(const int sock_fd);

int open_inet6_rx_mc_sock(const struct in6_addr mc_group,
			  const unsigned int port,
			  const unsigned int in_intf_idx);

void close_inet6_rx_mc_sock(const int sock_fd);

int open_inet_tx_mc_sock(const int mc_ttl,
			 const int no_mc_loop,
			 const struct in_addr out_intf_addr);

void close_inet_tx_mc_sock(int sock_fd);

int open_inet6_tx_mc_sock(const int mc_hops,
			  const int no_mc_loop,
			  const unsigned int out_intf_idx);

void close_inet6_tx_mc_sock(int sock_fd);

int inet_tx_mcast(const int sock_fd,
		  const void *pkt,
		  const size_t pkt_len,
 		  const struct sockaddr_in group_dests[],
		  const unsigned int group_dests_num);

int inet6_tx_mcast(const int sock_fd,
		   const void *pkt,
		   const size_t pkt_len,
 		   const struct sockaddr_in6 group_dests[],
		   const unsigned int group_dests_num);


int main(int argc, char *argv[])
{
	int in_sock_fd;
	int out_sock_fd;
	//struct in6_addr inet6_mcaddr;
	struct in_addr inet_mcaddr;
	struct in_addr in_intf_addr;
	struct in_addr out_intf_addr;
	struct sockaddr_in inet_out_mcaddr;
	const unsigned int pkt_buf_size = 0xffff;
	uint8_t pkt_buf[pkt_buf_size];
	unsigned int rx_pkt_len;


	/* IPv6 */
	//inet_pton(AF_INET6, "ff05::5", &inet6_mcaddr);
	//in_sock_fd = open_inet6_rx_mc_sock(inet6_mcaddr, 1234, 9); 

	/* IPv4 */
	inet_pton(AF_INET, "224.4.4.4", &inet_mcaddr);
	inet_pton(AF_INET, "1.1.1.1", &in_intf_addr);
	in_sock_fd = open_inet_rx_mc_sock(inet_mcaddr, 1234, in_intf_addr);
	if (in_sock_fd == -1) {
		perror("");
	}

	inet_pton(AF_INET, "1.1.1.1", &out_intf_addr);
	out_sock_fd = open_inet_tx_mc_sock(0, 0, out_intf_addr);

	memset(&inet_out_mcaddr, 0, sizeof(inet_out_mcaddr));
	inet_out_mcaddr.sin_family = AF_INET;
	inet_pton(AF_INET, "224.5.5.5", &inet_out_mcaddr.sin_addr);
	inet_out_mcaddr.sin_port = htons(1234);

	for ( ;; ) {
		rx_pkt_len = recv(in_sock_fd, pkt_buf, pkt_buf_size, 0);	
		if (rx_pkt_len > 0) {
			inet_tx_mcast(out_sock_fd, pkt_buf, rx_pkt_len,
				&inet_out_mcaddr, 1);
		}
	}

	/* IPv4 */
	close_inet_rx_mc_sock(in_sock_fd);
	close_inet_tx_mc_sock(out_sock_fd);

	return 0;

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


	close(sock_fd);

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


	close(sock_fd);

}


int open_inet_tx_mc_sock(const int mc_ttl,
			 const int no_mc_loop,
			 const struct in_addr out_intf_addr)
{
	int sock_fd;
	uint8_t ttl;
	uint8_t mc_loop;
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

	if (no_mc_loop == 1) {
		mc_loop = 0;
		ret = setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP,
			&mc_loop, sizeof(mc_loop));
		if (ret == -1) {
			return -1;
		}
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


	close(sock_fd);

}


int open_inet6_tx_mc_sock(const int mc_hops,
			  const int no_mc_loop,
			  const unsigned int out_intf_idx)
{
	int sock_fd;
	uint8_t hops;
	uint8_t mc_loop;
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

	if (no_mc_loop == 1) {
		mc_loop = 0;
		ret = setsockopt(sock_fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
			&mc_loop, sizeof(mc_loop));
		if (ret == -1) {
			return -1;
		}
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


	close(sock_fd);

}


int inet_tx_mcast(const int sock_fd,
		  const void *pkt,
		  const size_t pkt_len,
 		  const struct sockaddr_in group_dests[],
		  const unsigned int group_dests_num)
{
	unsigned int i;
	unsigned int tx_success = 0;
	ssize_t ret;


	for (i = 0; i < group_dests_num; i++) {
		ret = sendto(sock_fd, pkt, pkt_len, 0, 	
			(struct sockaddr *)&group_dests[i],
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
 		   const struct sockaddr_in6 group_dests[],
		   const unsigned int group_dests_num)
{
	unsigned int i;
	unsigned int tx_success = 0;
	ssize_t ret;


	for (i = 0; i < group_dests_num; i++) {
		ret = sendto(sock_fd, pkt, pkt_len, 0, 	
			(struct sockaddr *)&group_dests[i],
			sizeof(struct sockaddr_in6));
		if (ret != -1) {
			tx_success++;
		}
	}

	return tx_success;

}
