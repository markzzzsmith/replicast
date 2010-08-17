/*
 * replicast - replicate an incoming multicast stream to multiple destination
 *	       multicast streams
 *
 */

#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "global.h"
#include "log.h"
#include "aip_ptox.h"


struct inet_rx_mc_sock_params {
	struct in_addr mc_group;
	unsigned int port;
	struct in_addr in_intf_addr;	
};

struct inet_tx_mc_sock_params {
	unsigned int mc_ttl;
	unsigned int mc_loop;
	struct in_addr out_intf_addr;
	struct sockaddr_in *mc_dests;
	unsigned int mc_dests_num;	
};

struct inet6_rx_mc_sock_params {
	struct in6_addr mc_group;
	unsigned int port;
	unsigned int in_intf_idx;
};

struct inet6_tx_mc_sock_params {
	int mc_hops;
	unsigned int mc_loop;
	unsigned int out_intf_idx;
	struct sockaddr_in6 *mc_dests;
	unsigned int mc_dests_num;
};


struct program_parameters {
	struct inet_rx_mc_sock_params inet_rx_sock_parms;
	struct inet_tx_mc_sock_params inet_tx_sock_parms;
	struct inet6_rx_mc_sock_params inet6_rx_sock_parms;
	struct inet6_tx_mc_sock_params inet6_tx_sock_parms;
};

enum GLOBAL_DEFS {
	PKT_BUF_SIZE = 0xffff,
};


enum PARAM_ERRORS {
	PRMERR_NO_ERROR,
	PRMERR_NO_PARAMS,
};

enum REPLICAST_MODE {
	RCMODE_ERROR,
	RCMODE_INET_TO_INET,
	RCMODE_INET_TO_INET6,
	RCMODE_INET_TO_INET_INET6,
	RCMODE_INET6_TO_INET6,
	RCMODE_INET6_TO_INET,
	RCMODE_INET6_TO_INET_INET6,
};


enum REPLICAST_MODE get_prog_parms(int argc, char *argv[],
				   struct program_parameters *prog_parms,
				   enum PARAM_ERRORS **parm_error);

void init_prog_parms(struct program_parameters *prog_parms);

void get_prog_parms_cmdline(int argc, char *argv[],
			    struct program_parameters *prog_parms);

int str_to_inet(const int af,
		const char *str,
		struct in_addr *addr,
		unsigned int *port,
		char *intf,
		const unsigned int intf_len);
		

void close_sockets(void);

void inet_to_inet_mcast(int *inet_in_sock_fd,
			struct inet_rx_mc_sock_params rx_sock_parms,
			int *inet_out_sock_fd,
			struct inet_tx_mc_sock_params tx_sock_parms);

void inet_to_inet6_mcast(int *inet_in_sock_fd,
			struct inet_rx_mc_sock_params rx_sock_parms,
			int *inet6_out_sock_fd,
			struct inet6_tx_mc_sock_params tx_sock_parms);

void inet_to_inet_inet6_mcast(int *inet_in_sock_fd,
			      struct inet_rx_mc_sock_params rx_sock_parms,
			      int *inet_out_sock_fd,
			      struct inet_tx_mc_sock_params inet_tx_sock_parms,
			      int *inet6_out_sock_fd,
			      struct inet6_tx_mc_sock_params
							inet6_tx_sock_parms);

void inet6_to_inet6_mcast(int *inet6_in_sock_fd,
			  struct inet6_rx_mc_sock_params rx_sock_parms,
			  int *inet6_out_sock_fd,
			  struct inet6_tx_mc_sock_params tx_sock_parms);

void inet6_to_inet_mcast(int *inet6_in_sock_fd,
			 struct inet6_rx_mc_sock_params rx_sock_parms,
			 int *inet_out_sock_fd,
			 struct inet_tx_mc_sock_params tx_sock_parms);

void inet6_to_inet_inet6_mcast(int *inet6_in_sock_fd,
			       struct inet6_rx_mc_sock_params rx_sock_parms,
			       int *inet_out_sock_fd,
			       struct inet_tx_mc_sock_params inet_tx_sock_parms,
			       int *inet6_out_sock_fd,
			       struct inet6_tx_mc_sock_params
							inet6_tx_sock_parms);

void exit_errno(const char *func_name, const unsigned int linenum, int errnum);

int open_inet_rx_mc_sock(const struct in_addr mc_group,
			 const unsigned int port,
			 const struct in_addr in_intf_addr);

void close_inet_rx_mc_sock(const int sock_fd);

int open_inet6_rx_mc_sock(const struct in6_addr mc_group,
			  const unsigned int port,
			  const unsigned int in_intf_idx);

void close_inet6_rx_mc_sock(const int sock_fd);

int open_inet_tx_mc_sock(const unsigned int mc_ttl,
			 const unsigned int mc_loop,
			 const struct in_addr out_intf_addr);

void close_inet_tx_mc_sock(int sock_fd);

int open_inet6_tx_mc_sock(const int mc_hops,
			  const unsigned int mc_loop,
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
	//struct sockaddr_in inet_mc_dests[3];
	struct sockaddr_in *inet_mc_dests = NULL;
	struct inet6_rx_mc_sock_params rx6_sock_parms;
	struct inet6_tx_mc_sock_params tx6_sock_parms;
	struct sockaddr_in6 inet6_mc_dests[3];
	char out_if[30];
	char ap_err_str[AIP_STR_INET_MAX_LEN + 1];
	int ap_pton_inet_csv_ret;
	int ret;
	

	atexit(close_sockets);

	log_set_detail_level(LOG_SEV_DEBUG_LOW);


	aip_ptoh_inet(argv[1], &rx_sock_parms.mc_group,
		&rx_sock_parms.in_intf_addr, &rx_sock_parms.port, NULL);
	if (rx_sock_parms.in_intf_addr.s_addr == INADDR_NONE) {
		rx_sock_parms.in_intf_addr.s_addr = INADDR_ANY;
	}

	ap_pton_inet_csv_ret = ap_pton_inet_csv(argv[2], &inet_mc_dests, 0,
		(char *)&ap_err_str, AIP_STR_INET_MAX_LEN + 1);

	printf("ap_pton_inet_csv_ret = %d\n", ap_pton_inet_csv_ret);
	printf("ap_err_str = %s\n", ap_err_str);

	tx_sock_parms.mc_ttl = 1;
	tx_sock_parms.mc_loop = 1;
	//inet_pton(AF_INET, "1.1.1.1", &tx_sock_parms.out_intf_addr);
	tx_sock_parms.out_intf_addr.s_addr = INADDR_ANY; /* use route table */

/*
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
*/

	tx_sock_parms.mc_dests = inet_mc_dests;
	tx_sock_parms.mc_dests_num = ap_pton_inet_csv_ret;
	
	inet_to_inet_mcast(&inet_in_sock_fd, rx_sock_parms, &inet_out_sock_fd,
		tx_sock_parms);

	printf("argv[1] = %s\n", argv[1]);
	ret = aip_ptoh_inet6(argv[1], &rx6_sock_parms.mc_group,
		&rx6_sock_parms.in_intf_idx, &rx6_sock_parms.port, NULL);
	printf("ret = %d\n", ret);

/*
	inet_pton(AF_INET6, "ff05::30", &rx6_sock_parms.mc_group);
	rx6_sock_parms.port = 1234; 
	rx6_sock_parms.in_intf_idx = 1;
*/

	tx6_sock_parms.mc_hops = 1;
	tx6_sock_parms.mc_loop = 1;
	//tx6_sock_parms.out_intf_idx = 2; /* eth0 */
	tx6_sock_parms.out_intf_idx = 0; /* any - looks to match on 'local' route table */

	memset(inet6_mc_dests, 0, sizeof(struct sockaddr_in6) * 3);

	inet6_mc_dests[0].sin6_family = AF_INET6;	
	inet_pton(AF_INET6, "ff02::15", &inet6_mc_dests[0].sin6_addr);
	inet6_mc_dests[0].sin6_port = htons(1234);

	inet6_mc_dests[1].sin6_family = AF_INET6;	
	inet_pton(AF_INET6, "ff02::16", &inet6_mc_dests[1].sin6_addr);
	inet6_mc_dests[1].sin6_port = htons(1234);

	inet6_mc_dests[2].sin6_family = AF_INET6;	
	inet_pton(AF_INET6, "ff02::17", &inet6_mc_dests[2].sin6_addr);
	inet6_mc_dests[2].sin6_port = htons(1234);

	tx6_sock_parms.mc_dests = inet6_mc_dests;
	tx6_sock_parms.mc_dests_num = 3;

/*
	inet_to_inet6_mcast(&inet_in_sock_fd, rx_sock_parms,
		&inet6_out_sock_fd, tx6_sock_parms);
*/

/*
	inet_to_inet_inet6_mcast(&inet_in_sock_fd, rx_sock_parms,
				 &inet_out_sock_fd, tx_sock_parms,
				 &inet6_out_sock_fd, tx6_sock_parms);

*/


	inet6_to_inet6_mcast(&inet6_in_sock_fd, rx6_sock_parms,
			     &inet6_out_sock_fd, tx6_sock_parms);



/*
	inet6_to_inet_mcast(&inet6_in_sock_fd, rx6_sock_parms,
			     &inet_out_sock_fd, tx_sock_parms);
 */

	return 0;

}


enum REPLICAST_MODE get_prog_parms(int argc, char *argv[],
				   struct program_parameters *prog_parms,
				   enum PARAM_ERRORS **parm_error)
{

	init_prog_parms(prog_parms);

	get_prog_parms_cmdline(argc, argv, prog_parms);

	*parm_error = PRMERR_NO_ERROR;
	return RCMODE_ERROR;

}


void init_prog_parms(struct program_parameters *prog_parms)
{


	prog_parms->inet_rx_sock_parms.mc_group.s_addr = INADDR_NONE;
	prog_parms->inet_rx_sock_parms.port = 0;
	prog_parms->inet_rx_sock_parms.in_intf_addr.s_addr = INADDR_NONE;

	prog_parms->inet_tx_sock_parms.mc_ttl = 1;
	prog_parms->inet_tx_sock_parms.mc_loop = 0;
	prog_parms->inet_tx_sock_parms.out_intf_addr.s_addr = INADDR_NONE;
	prog_parms->inet_tx_sock_parms.mc_dests = NULL;
	prog_parms->inet_tx_sock_parms.mc_dests_num = 0;

	memcpy(&prog_parms->inet6_rx_sock_parms.mc_group, &in6addr_any,
		sizeof(in6addr_any));
	prog_parms->inet6_rx_sock_parms.port = 0;
	prog_parms->inet6_rx_sock_parms.in_intf_idx = 0;
	
	prog_parms->inet6_tx_sock_parms.mc_hops = 1;
	prog_parms->inet6_tx_sock_parms.mc_loop = 0;
	prog_parms->inet6_tx_sock_parms.out_intf_idx = 0;
	prog_parms->inet6_tx_sock_parms.mc_dests = NULL;
	prog_parms->inet6_tx_sock_parms.mc_dests_num = 0;

}


void get_prog_parms_cmdline(int argc, char *argv[],
			    struct program_parameters *prog_parms)
{
	enum CMDLINE_OPTS {
		CMDLINE_OPT_HELP = 1,
		CMDLINE_OPT_4SG,
		CMDLINE_OPT_4TTL,
		CMDLINE_OPT_4LOOP,
		CMDLINE_OPT_4OUTF,
		CMDLINE_OPT_4DGS,
		CMDLINE_OPT_6SG,
	};
	struct option cmdline_opts[] = {
		{"help", no_argument, NULL, CMDLINE_OPT_HELP},
		{"4sg", required_argument, NULL, CMDLINE_OPT_4SG},
		{"4ttl", required_argument, NULL, CMDLINE_OPT_4TTL},
		{"4loop", required_argument, NULL, CMDLINE_OPT_4LOOP},
		{"4outf", requited_argument, NULL, CMDLINE_OPT_4OUTF},
		{"4dgs", required_argument, NULL, CMDLINE_OPT_4DGS},
		{"6sg", required_argument, NULL, CMDLINE_OPT_6SG},
		{0, 0, 0, 0}
	};





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


	log_debug("%s() entry\n", __func__);

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
		log_debug_low("%s(): recv() == %d\n", __func__, rx_pkt_len);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		if (rx_pkt_len > 0) {
			inet_tx_mcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms.mc_dests,
				tx_sock_parms.mc_dests_num);
		}
	}

}


void inet_to_inet6_mcast(int *inet_in_sock_fd,
			struct inet_rx_mc_sock_params rx_sock_parms,
			int *inet6_out_sock_fd,
			struct inet6_tx_mc_sock_params tx_sock_parms)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;


	log_debug("%s() entry\n", __func__);

	*inet_in_sock_fd = open_inet_rx_mc_sock(rx_sock_parms.mc_group,
		rx_sock_parms.port, rx_sock_parms.in_intf_addr);
	if (*inet_in_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet6_out_sock_fd = open_inet6_tx_mc_sock(tx_sock_parms.mc_hops,
		tx_sock_parms.mc_loop, tx_sock_parms.out_intf_idx);
	if (*inet6_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	for ( ;; ) {
		rx_pkt_len = recv(*inet_in_sock_fd, pkt_buf, PKT_BUF_SIZE, 0);
		log_debug_low("%s(): recv() == %d\n", __func__, rx_pkt_len);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		if (rx_pkt_len > 0) {
			inet6_tx_mcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms.mc_dests,
				tx_sock_parms.mc_dests_num);
		}
	}

}


void inet_to_inet_inet6_mcast(int *inet_in_sock_fd,
			      struct inet_rx_mc_sock_params rx_sock_parms,
			      int *inet_out_sock_fd,
			      struct inet_tx_mc_sock_params inet_tx_sock_parms,
			      int *inet6_out_sock_fd,
			      struct inet6_tx_mc_sock_params
							inet6_tx_sock_parms)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;


	log_debug("%s() entry\n", __func__);

	*inet_in_sock_fd = open_inet_rx_mc_sock(rx_sock_parms.mc_group,
		rx_sock_parms.port, rx_sock_parms.in_intf_addr);
	if (*inet_in_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet_out_sock_fd = open_inet_tx_mc_sock(inet_tx_sock_parms.mc_ttl,
		inet_tx_sock_parms.mc_loop, inet_tx_sock_parms.out_intf_addr);
	if (*inet_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet6_out_sock_fd = open_inet6_tx_mc_sock(inet6_tx_sock_parms.mc_hops,
		inet6_tx_sock_parms.mc_loop, inet6_tx_sock_parms.out_intf_idx);
	if (*inet6_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	for ( ;; ) {
		rx_pkt_len = recv(*inet_in_sock_fd, pkt_buf, PKT_BUF_SIZE, 0);
		if (rx_pkt_len > 0) {
			inet_tx_mcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				inet_tx_sock_parms.mc_dests,
				inet_tx_sock_parms.mc_dests_num);
			inet6_tx_mcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				inet6_tx_sock_parms.mc_dests,
				inet6_tx_sock_parms.mc_dests_num);
		}
	}

}


void inet6_to_inet6_mcast(int *inet6_in_sock_fd,
			  struct inet6_rx_mc_sock_params rx_sock_parms,
			  int *inet6_out_sock_fd,
			  struct inet6_tx_mc_sock_params tx_sock_parms)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;


	log_debug("%s() entry\n", __func__);

	*inet6_in_sock_fd = open_inet6_rx_mc_sock(rx_sock_parms.mc_group,
		rx_sock_parms.port, rx_sock_parms.in_intf_idx);
	if (*inet6_in_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet6_out_sock_fd = open_inet6_tx_mc_sock(tx_sock_parms.mc_hops,
		tx_sock_parms.mc_loop, tx_sock_parms.out_intf_idx);
	if (*inet6_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	for ( ;; ) {
		rx_pkt_len = recv(*inet6_in_sock_fd, pkt_buf, PKT_BUF_SIZE, 0);
		log_debug_low("%s(): recv() == %d\n", __func__, rx_pkt_len);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		if (rx_pkt_len > 0) {
			inet6_tx_mcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms.mc_dests,
				tx_sock_parms.mc_dests_num);
		}
	}

}


void inet6_to_inet_mcast(int *inet6_in_sock_fd,
			 struct inet6_rx_mc_sock_params rx_sock_parms,
			 int *inet_out_sock_fd,
			 struct inet_tx_mc_sock_params tx_sock_parms)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;


	log_debug("%s() entry\n", __func__);

	*inet6_in_sock_fd = open_inet6_rx_mc_sock(rx_sock_parms.mc_group,
		rx_sock_parms.port, rx_sock_parms.in_intf_idx);
	if (*inet6_in_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet_out_sock_fd = open_inet_tx_mc_sock(tx_sock_parms.mc_ttl,
		tx_sock_parms.mc_loop, tx_sock_parms.out_intf_addr);
	if (*inet_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	for ( ;; ) {
		rx_pkt_len = recv(*inet6_in_sock_fd, pkt_buf, PKT_BUF_SIZE, 0);
		log_debug_low("%s(): recv() == %d\n", __func__, rx_pkt_len);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		if (rx_pkt_len > 0) {
			inet_tx_mcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms.mc_dests,
				tx_sock_parms.mc_dests_num);
		}
	}

}

void inet6_to_inet_inet6_mcast(int *inet6_in_sock_fd,
			       struct inet6_rx_mc_sock_params rx_sock_parms,
			       int *inet_out_sock_fd,
			       struct inet_tx_mc_sock_params inet_tx_sock_parms,
			       int *inet6_out_sock_fd,
			       struct inet6_tx_mc_sock_params
							inet6_tx_sock_parms)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;


	log_debug("%s() entry\n", __func__);

	*inet6_in_sock_fd = open_inet6_rx_mc_sock(rx_sock_parms.mc_group,
		rx_sock_parms.port, rx_sock_parms.in_intf_idx);
	if (*inet6_in_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet_out_sock_fd = open_inet_tx_mc_sock(inet_tx_sock_parms.mc_ttl,
		inet_tx_sock_parms.mc_loop, inet_tx_sock_parms.out_intf_addr);
	if (*inet_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet6_out_sock_fd = open_inet6_tx_mc_sock(inet6_tx_sock_parms.mc_hops,
		inet6_tx_sock_parms.mc_loop, inet6_tx_sock_parms.out_intf_idx);
	if (*inet6_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	for ( ;; ) {
		rx_pkt_len = recv(*inet6_in_sock_fd, pkt_buf, PKT_BUF_SIZE, 0);
		if (rx_pkt_len > 0) {
			inet_tx_mcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				inet_tx_sock_parms.mc_dests,
				inet_tx_sock_parms.mc_dests_num);
			inet6_tx_mcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				inet6_tx_sock_parms.mc_dests,
				inet6_tx_sock_parms.mc_dests_num);
		}
	}

}


void exit_errno(const char *func_name, const unsigned int linenum, int errnum)
{


	log_debug("%s() entry\n", __func__);

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
	int one = 1;
	struct sockaddr_in sa_in_mcaddr;
	struct ip_mreq ip_mcast_req;


	log_debug("%s() entry\n", __func__);

	/* socket() */
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		return -1;
	}

	/* allow duplicate binds to group and UDP port */
	ret = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &one,
		sizeof(one));
	if (ret == -1) {
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


	log_debug("%s() entry\n", __func__);

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
	int one = 1;
	struct sockaddr_in6 sa_in6_mcaddr;
	struct ipv6_mreq ipv6_mcast_req;


	log_debug("%s() entry\n", __func__);

	/* socket() */
	sock_fd = socket(AF_INET6, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		log_debug_low("%s(): socket() == %d\n", __func__, sock_fd);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		return -1;
	}

	/* allow duplicate binds to group and UDP port */
	ret = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &one,
		sizeof(one));
	if (ret == -1) {
		log_debug_low("%s(): setsockopt(SO_REUSEADDR) == %d\n",
			__func__, ret);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		return -1;
	}

	/* bind() to UDP port*/
	memset(&sa_in6_mcaddr, 0, sizeof(sa_in6_mcaddr));
	sa_in6_mcaddr.sin6_family = AF_INET6;
	sa_in6_mcaddr.sin6_addr = mc_group;
	sa_in6_mcaddr.sin6_port = htons(port);
	if (IN6_IS_ADDR_MC_LINKLOCAL(&sa_in6_mcaddr.sin6_addr)) {
		sa_in6_mcaddr.sin6_scope_id = in_intf_idx;
	}
	ret = bind(sock_fd, (struct sockaddr *) &sa_in6_mcaddr,
		sizeof(sa_in6_mcaddr));
	if (ret == -1) {
		log_debug_low("%s(): bind() == %d\n", __func__, ret);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		return -1;
	}

	/* setsockopt() to join mcast group */
	ipv6_mcast_req.ipv6mr_multiaddr = mc_group;
	ipv6_mcast_req.ipv6mr_interface = in_intf_idx;
	ret = setsockopt(sock_fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
		&ipv6_mcast_req, sizeof(ipv6_mcast_req));
	if (ret == -1) {
		log_debug_low("%s(): setsockopt(IPV6_ADD_MEMBERSHIP) == %d\n",
			__func__, ret);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		return -1;
	}

	return sock_fd;

}


void close_inet6_rx_mc_sock(const int sock_fd)
{


	log_debug("%s() entry\n", __func__);

	if (sock_fd != -1) {
		close(sock_fd);
	}

}


int open_inet_tx_mc_sock(const unsigned int mc_ttl,
			 const unsigned int mc_loop,
			 const struct in_addr out_intf_addr)
{
	int sock_fd;
	uint8_t ttl;
	uint8_t mcloop;
	int ret;


	log_debug("%s() entry\n", __func__);

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


	log_debug("%s() entry\n", __func__);

	if (sock_fd != -1) {
		close(sock_fd);
	}

}


int open_inet6_tx_mc_sock(const int mc_hops,
			  const unsigned int mc_loop,
			  const unsigned int out_intf_idx)
{
	int sock_fd;
	int ret;


	log_debug("%s() entry\n", __func__);

	sock_fd = socket(AF_INET6, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		return -1;
	}

	if (mc_hops > 0) {
		ret = setsockopt(sock_fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
			&mc_hops, sizeof(mc_hops));	
		if (ret == -1) {
			return -1;
		}
	}

	ret = setsockopt(sock_fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &mc_loop,
		sizeof(mc_loop));
	if (ret == -1) {
		return -1;
	}

	ret = setsockopt(sock_fd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
		&out_intf_idx, sizeof(out_intf_idx));
	if (ret == -1) {
		return -1;
	}

	return sock_fd;
			
}


void close_inet6_tx_mc_sock(int sock_fd)
{


	log_debug("%s() entry\n", __func__);

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


	log_debug("%s() entry\n", __func__);

	for (i = 0; i < mc_dests_num; i++) {
		ret = sendto(sock_fd, pkt, pkt_len, 0, 	
			(struct sockaddr *)&inet_mc_dests[i],
			sizeof(struct sockaddr_in));
		if (ret != -1) {
			tx_success++;
		}
		log_debug_low("%s(): sendto() == %d\n", __func__, ret);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
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


	log_debug("%s() entry\n", __func__);

	for (i = 0; i < mc_dests_num; i++) {
		ret = sendto(sock_fd, pkt, pkt_len, 0, 	
			(struct sockaddr *)&inet6_mc_dests[i],
			sizeof(struct sockaddr_in6));
		if (ret != -1) {
			tx_success++;
		}
		log_debug_low("%s(): sendto() == %d\n", __func__, ret);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
	}

	return tx_success;

}
