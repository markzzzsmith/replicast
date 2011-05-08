/*
 * replicast - replicate an incoming multicast or unicast stream to multiple
 * destination multicast or unicast streams
 *
 * Copyright (C) 2011 Mark Smith <markzzzsmith@yahoo.com.au>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA. 
 *
 */

#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "log.h"
#include "inetaddr.h"


enum GLOBAL_DEFS {
	PKT_BUF_SIZE = 0xffff,
};

enum VALIDATE_PROG_OPTS {
	VPO_HELP,
	VPO_LICENSE,
	VPO_ERR_UNKNOWN_OPT,
	VPO_ERR_NO_SRC_ADDR,
	VPO_ERR_MULTI_SRC_ADDRS,
	VPO_ERR_NO_DST_ADDRS,
	VPO_MODE_INETINETINET6,
	VPO_MODE_INETINET,
	VPO_MODE_INETINET6,
	VPO_MODE_INET6INETINET6,
	VPO_MODE_INET6INET,
	VPO_MODE_INET6INET6,
	VPO_ERR_UNKNOWN,
};

enum VALIDATE_PROG_OPTS_VALS {
	VPOV_ERR_SRC_ADDR,
	VPOV_ERR_IF_ADDR,
	VPOV_ERR_SRC_PORT,
	VPOV_ERR_DST_PORT,
	VPOV_ERR_INET_DST_ADDR,
	VPOV_ERR_INET_TX_MCTTL_RANGE,
	VPOV_ERR_INET_OUT_INTF,
	VPOV_ERR_INET6_OUT_INTF,
	VPOV_ERR_INET6_DST_ADDR,
	VPOV_ERR_INET6_TX_HOPS_RANGE,
	VPOV_ERR_UNKNOWN,
	VPOV_ERR_MEMORY,
	VPOV_OPTS_VALS_VALID,
};

enum OPT_ERR {
	OE_UNKNOWN_OPT,
	OE_NO_SRC_ADDR,
	OE_MULTI_SRC_ADDRS,
	OE_NO_DST_ADDRS,
	OE_SRC_GRP_ADDR,
	OE_IF_ADDR,
	OE_SRC_PORT,
	OE_DST_PORT,
	OE_INET_DST_ADDR,
	OE_INET_TX_MCTTL_RANGE,
	OE_INET_OUT_INTF,
	OE_INET6_OUT_INTF,
	OE_INET6_DST_ADDR,
	OE_INET6_TX_HOPS_RANGE,
	OE_MEMORY_ERROR,
	OE_UNKNOWN_ERROR,
};


enum REPLICAST_MODE {
	RCMODE_UNKNOWN,
	RCMODE_HELP,
	RCMODE_LICENSE,
	RCMODE_ERROR,
	RCMODE_INET_TO_INET,
	RCMODE_INET_TO_INET6,
	RCMODE_INET_TO_INET_INET6,
	RCMODE_INET6_TO_INET6,
	RCMODE_INET6_TO_INET,
	RCMODE_INET6_TO_INET_INET6,
};



struct inet_rx_sock_params {
	struct in_addr rx_addr;
	unsigned int port;
	struct in_addr in_intf_addr;	
};

struct inet_tx_sock_params {
	unsigned int mc_ttl;
	unsigned int mc_loop;
	struct in_addr out_intf_addr;
	struct sockaddr_in *dests;
	unsigned int dests_num;	
	unsigned int mc_dests_num;
};

struct inet6_rx_sock_params {
	struct in6_addr rx_addr;
	unsigned int port;
	unsigned int in_intf_idx;
};

struct inet6_tx_sock_params {
	int mc_hops;
	unsigned int mc_loop;
	unsigned int out_intf_idx;
	struct sockaddr_in6 *dests;
	unsigned int dests_num;
	unsigned int mc_dests_num;
};

struct socket_fds {
	int inet_in_sock_fd;
	int inet6_in_sock_fd;
	int inet_out_sock_fd;
	int inet6_out_sock_fd;
};

struct packet_counters {
	unsigned long long inet_in_pkts;
	unsigned long long inet6_in_pkts;
	unsigned long long inet_out_pkts;
	unsigned long long inet6_out_pkts;
};

struct program_options {
	unsigned int help_set;
	unsigned int license_set;
	unsigned int unknown_opt_set;
	char *unknown_opt_str;

	unsigned int no_daemon_set;

	unsigned int inet_rx_sock_mcgroup_set;
	char *inet_rx_sock_mcgroup_str;

	unsigned int inet_tx_sock_mc_ttl_set;
	char *inet_tx_sock_mc_ttl_str;
	unsigned int inet_tx_sock_mc_loop_set;
	unsigned int inet_tx_sock_out_intf_set;
	char *inet_tx_sock_out_intf_str;
	unsigned int inet_tx_sock_dests_set;
	char *inet_tx_sock_dests_str;

	unsigned int inet6_rx_sock_mcgroup_set;
	char *inet6_rx_sock_mcgroup_str;

	unsigned int inet6_tx_sock_mc_hops_set;
	char *inet6_tx_sock_mc_hops_str;
	unsigned int inet6_tx_sock_mc_loop_set;
	unsigned int inet6_tx_sock_out_intf_set;
	char *inet6_tx_sock_out_intf_str;
	unsigned int inet6_tx_sock_dests_set;
	char *inet6_tx_sock_dests_str;
};


struct program_parameters {
	enum REPLICAST_MODE rc_mode;
	unsigned int become_daemon;
	struct inet_rx_sock_params inet_rx_sock_parms;
	struct inet_tx_sock_params inet_tx_sock_parms;
	struct inet6_rx_sock_params inet6_rx_sock_parms;
	struct inet6_tx_sock_params inet6_tx_sock_parms;
};


void log_prog_help(void);

void log_prog_license(void);

void get_prog_parms(int argc, char *argv[],
		    struct program_parameters *prog_parms,
		    char err_str[],
		    const unsigned int err_str_size);

void init_prog_opts(struct program_options *prog_opts);

void init_prog_parms(struct program_parameters *prog_parms);

void get_prog_opts_cmdline(int argc, char *argv[],
			   struct program_options *prog_opts);

enum VALIDATE_PROG_OPTS validate_prog_opts(
				const struct program_options *prog_opts);

enum VALIDATE_PROG_OPTS_VALS validate_prog_opts_vals(
				const struct program_options *prog_opts,
				struct program_parameters *prog_parms,
				char *err_str_parm,
				const unsigned int err_str_size);

int validate_prog_opts_values(const struct program_options *prog_opts,
			      struct program_parameters *prog_parms,
			      char *err_str_parm,
			      const unsigned int err_str_size);

void log_prog_banner(void);

void log_prog_parms(const struct program_parameters *prog_parms);

void log_inet_rx_sock_parms(const struct inet_rx_sock_params
								*inet_rx_parms);

void log_inet6_rx_sock_parms(const struct inet6_rx_sock_params
							*inet6_rx_parms);

void log_inet_tx_sock_parms(const struct inet_tx_sock_params
							*inet_tx_parms);

void log_inet6_tx_sock_parms(const struct inet6_tx_sock_params
							*inet6_tx_parms);

void log_opt_error(enum OPT_ERR option_err,
		   const char *err_str_parm);


void cleanup_prog_parms(struct program_parameters *prog_parms);

void init_sock_fds(struct socket_fds *sock_fds);

void inet_to_inet_rcast(int *inet_in_sock_fd,
			const struct inet_rx_sock_params *rx_sock_parms,
			int *inet_out_sock_fd,
			const struct inet_tx_sock_params *tx_sock_parms,
			struct packet_counters *pkt_counters);

void inet_to_inet6_rcast(int *inet_in_sock_fd,
			 const struct inet_rx_sock_params *rx_sock_parms,
			 int *inet6_out_sock_fd,
			 const struct inet6_tx_sock_params *tx_sock_parms,
			 struct packet_counters *pkt_counters);

void inet_to_inet_inet6_rcast(int *inet_in_sock_fd,
			      const struct inet_rx_sock_params *rx_sock_parms,
			      int *inet_out_sock_fd,
			      const struct inet_tx_sock_params
							*inet_tx_sock_parms,
			      int *inet6_out_sock_fd,
			      const struct inet6_tx_sock_params
							*inet6_tx_sock_parms,
			      struct packet_counters *pkt_counters);

void inet6_to_inet6_rcast(int *inet6_in_sock_fd,
			  const struct inet6_rx_sock_params *rx_sock_parms,
			  int *inet6_out_sock_fd,
			  const struct inet6_tx_sock_params *tx_sock_parms,
			  struct packet_counters *pkt_counters);

void inet6_to_inet_rcast(int *inet6_in_sock_fd,
			 const struct inet6_rx_sock_params *rx_sock_parms,
			 int *inet_out_sock_fd,
			 const struct inet_tx_sock_params *tx_sock_parms,
			 struct packet_counters *pkt_counters);

void inet6_to_inet_inet6_rcast(int *inet6_in_sock_fd,
			       const struct inet6_rx_sock_params *rx_sock_parms,
			       int *inet_out_sock_fd,
			       const struct inet_tx_sock_params
							*inet_tx_sock_parms,
			       int *inet6_out_sock_fd,
			       const struct inet6_tx_sock_params
							*inet6_tx_sock_parms,
			       struct packet_counters *pkt_counters);

void exit_errno(const char *func_name, const unsigned int linenum, int errnum);

void init_packet_counters(struct packet_counters *pkt_counters);

void daemonise(void);

void create_child_process(void);

void create_new_session(void);

void change_to_rootdir(void);

void open_syslog_log(void);

void close_stdfiles(void);

void install_exit_signal_handlers(void);

void install_usr_signal_handlers(void);

void install_sigterm_handler(struct sigaction *sigterm_action);

void install_sigint_handler(struct sigaction *sigint_action);

void install_sigusr1_handler(struct sigaction *sigusr1_action);

void install_sigusr2_handler(struct sigaction *sigusr2_action);

void sigterm_handler(int signum);

void sigint_handler(int signum);

void sigusr1_handler(int signum);

void sigusr2_handler(int signum);

int open_inet_rx_sock(const struct inet_rx_sock_params *sock_parms);

void close_inet_rx_sock(const int sock_fd);

int open_inet6_rx_sock(const struct inet6_rx_sock_params *sock_parms);

void close_inet6_rx_sock(const int sock_fd);

int open_inet_tx_sock(const struct inet_tx_sock_params *sock_parms);

void close_inet_tx_sock(int sock_fd);

int open_inet6_tx_sock(const struct inet6_tx_sock_params *sock_parms);

void close_inet6_tx_sock(int sock_fd);

int inet_tx_rcast(const int sock_fd,
		  const void *pkt,
		  const size_t pkt_len,
 		  const struct sockaddr_in inet_dests[]);

int inet6_tx_rcast(const int sock_fd,
		   const void *pkt,
		   const size_t pkt_len,
 		   const struct sockaddr_in6 inet6_dests[]);

void close_sockets(const struct socket_fds *sock_fds);

void log_packet_counters(const enum REPLICAST_MODE rc_mode,
			 const struct packet_counters *pkt_counters);

void exit_program(void);

struct socket_fds sock_fds;

struct program_parameters prog_parms;

const float replicast_version = 0.1;
const char *program_name = "replicast";

const size_t syslog_ident_len = 100;
char syslog_ident[100];

struct sigaction sigterm_action;
struct sigaction sigint_action;
struct sigaction sigusr1_action;
struct sigaction sigusr2_action;

struct packet_counters pkt_counters;


int main(int argc, char *argv[])
{
	char *err_str = NULL;


	log_set_detail_level(LOG_SEV_DEBUG_LOW);

	init_prog_parms(&prog_parms);

	init_sock_fds(&sock_fds);

	init_packet_counters(&pkt_counters);

	install_exit_signal_handlers();

	get_prog_parms(argc, argv, &prog_parms, err_str, 0);

	switch (prog_parms.rc_mode) {
	case RCMODE_HELP:
		log_debug_med("%s() rc_mode = RCMODE_HELP\n", __func__);
		log_prog_banner();
		log_prog_help();
		break;
	case RCMODE_LICENSE:
		log_debug_med("%s() rc_mode = RCMODE_LICENSE\n", __func__);
		log_prog_banner();
		log_prog_license();
		break;
	case RCMODE_INET_TO_INET:
		log_debug_med("%s() rc_mode = RCMODE_INET_TO_INET\n", __func__);
		install_usr_signal_handlers();
		if (prog_parms.become_daemon) {
			daemonise();
		}
		log_prog_banner();
		log_prog_parms(&prog_parms);
		inet_to_inet_rcast(&sock_fds.inet_in_sock_fd,
				   &prog_parms.inet_rx_sock_parms,
				   &sock_fds.inet_out_sock_fd,
				   &prog_parms.inet_tx_sock_parms,
				   &pkt_counters);
		break;
	case RCMODE_INET_TO_INET6:
		log_debug_med("%s() rc_mode = RCMODE_INET_TO_INET6\n",
								__func__);
		install_usr_signal_handlers();
		if (prog_parms.become_daemon) {
			daemonise();
		}
		log_prog_banner();
		log_prog_parms(&prog_parms);
		inet_to_inet6_rcast(&sock_fds.inet_in_sock_fd,
				    &prog_parms.inet_rx_sock_parms,
				    &sock_fds.inet6_out_sock_fd,
				    &prog_parms.inet6_tx_sock_parms,
				    &pkt_counters);
		break;
	case RCMODE_INET_TO_INET_INET6:
		log_debug_med("%s() rc_mode = RCMODE_INET_TO_INET_INET6\n",
								__func__);
		install_usr_signal_handlers();
		if (prog_parms.become_daemon) {
			daemonise();
		}
		log_prog_banner();
		log_prog_parms(&prog_parms);
		inet_to_inet_inet6_rcast(&sock_fds.inet_in_sock_fd,
					 &prog_parms.inet_rx_sock_parms,
					 &sock_fds.inet_out_sock_fd,
					 &prog_parms.inet_tx_sock_parms,
					 &sock_fds.inet6_out_sock_fd,
					 &prog_parms.inet6_tx_sock_parms,
					 &pkt_counters);
		break;
	case RCMODE_INET6_TO_INET6:
		log_debug_med("%s() rc_mode = RCMODE_INET6_TO_INET6\n",
								__func__);
		install_usr_signal_handlers();
		if (prog_parms.become_daemon) {
			daemonise();
		}
		log_prog_banner();
		log_prog_parms(&prog_parms);
		inet6_to_inet6_rcast(&sock_fds.inet6_in_sock_fd,
				     &prog_parms.inet6_rx_sock_parms,
				     &sock_fds.inet6_out_sock_fd,
				     &prog_parms.inet6_tx_sock_parms,
				     &pkt_counters);
		break;
	case RCMODE_INET6_TO_INET:
		log_debug_med("%s() rc_mode = RCMODE_INET6_TO_INET\n",
								__func__);
		install_usr_signal_handlers();
		if (prog_parms.become_daemon) {
			daemonise();
		}
		log_prog_banner();
		log_prog_parms(&prog_parms);
		inet6_to_inet_rcast(&sock_fds.inet6_in_sock_fd,
				    &prog_parms.inet6_rx_sock_parms,
				    &sock_fds.inet_out_sock_fd,
				    &prog_parms.inet_tx_sock_parms,
				    &pkt_counters);
		break;
	case RCMODE_INET6_TO_INET_INET6:
		log_debug_med("%s() rc_mode = RCMODE_INET6_TO_INET_INET6\n",
								__func__);
		install_usr_signal_handlers();
		if (prog_parms.become_daemon) {
			daemonise();
		}
		log_prog_banner();
		log_prog_parms(&prog_parms);
		inet6_to_inet_inet6_rcast(&sock_fds.inet6_in_sock_fd,
				    &prog_parms.inet6_rx_sock_parms,
				    &sock_fds.inet_out_sock_fd,
				    &prog_parms.inet_tx_sock_parms,
				    &sock_fds.inet6_out_sock_fd,
				    &prog_parms.inet6_tx_sock_parms,
				    &pkt_counters);
		break;
	case RCMODE_ERROR:
		log_debug_med("%s() rc_mode = RCMODE_ERROR\n",
								__func__);
		exit(EXIT_FAILURE);
		break;
	default:
		break;
	}

	return EXIT_FAILURE;

}


void log_prog_help(void)
{


	log_debug_med("%s() entry\n", __func__);

	log_msg(LOG_SEV_INFO, "\nreplicate IPv4 or IPv6 UDP datagrams to "
		"IPv4 and/or IPv6 destinations.\n");

	log_msg(LOG_SEV_INFO, "\ncommand line options:\n");

	log_msg(LOG_SEV_INFO, "-help\n");
	log_msg(LOG_SEV_INFO, "-license\n");
	log_msg(LOG_SEV_INFO, "-nodaemon\n");

	log_msg(LOG_SEV_INFO, "-4in <addr>[%<ifname>|<ifaddr>]:<port>\n");
	log_msg(LOG_SEV_INFO, "\te.g. -4in 224.0.0.35:1234\n");
	log_msg(LOG_SEV_INFO, "\te.g. -4in 224.0.0.35%%eth0:1234\n");
	log_msg(LOG_SEV_INFO, "\te.g. -4in 224.0.0.35%%192.168.1.1:1234\n");
	log_msg(LOG_SEV_INFO, "\te.g. -4in 192.168.1.25:1234\n");
	log_msg(LOG_SEV_INFO, "\te.g. -4in 0.0.0.0:1234\n");

	log_msg(LOG_SEV_INFO, "-6in <\\[addr\\]>[%<ifname>]:<port>\n");
	log_msg(LOG_SEV_INFO, "\te.g. -6in [ff05::35]:1234\n");
	log_msg(LOG_SEV_INFO, "\te.g. -6in [ff05::35%%eth0]:1234\n");
	log_msg(LOG_SEV_INFO, "\te.g. -6in [fe80::1%%eth0]:1234\n");
	log_msg(LOG_SEV_INFO, "\te.g. -6in [2001:db8::1]:1234\n");
	log_msg(LOG_SEV_INFO, "\te.g. -6in [::]:1234\n");


	log_msg(LOG_SEV_INFO, "-4out <addr>:<port>,<addr>:port,...\n");
	log_msg(LOG_SEV_INFO, "\te.g. -4out 224.0.0.36:1234,");
	log_msg(LOG_SEV_INFO, "224.0.0.37:5678,192.168.1.1:9012\n");

	log_msg(LOG_SEV_INFO, "-4mcttl <ttl> - outgoing multicast TTL. "
		"default is 1.\n");
	log_msg(LOG_SEV_INFO, "\te.g. -4mcttl 32\n");

	log_msg(LOG_SEV_INFO, "-4mcloop - loop multicast to localhost.\n");

	log_msg(LOG_SEV_INFO, "-4mcoutif <ifname> - multicast output "
		"interface.\n");
	log_msg(LOG_SEV_INFO, "\te.g. -4mcoutif eth0\n");

	log_msg(LOG_SEV_INFO, "-6out <\\[addr\\]>:<port>,<\\[addr\\]>:<port>,"
		"...\n");
	log_msg(LOG_SEV_INFO, "\te.g. -6out [ff05::36]:1234,");
	log_msg(LOG_SEV_INFO, "[ff05::37]:5678,[2001:db8::1]:9012\n");

	log_msg(LOG_SEV_INFO, "-6mchops <hop count> - outgoing multicast"
		" hops. default is 1.\n");
	log_msg(LOG_SEV_INFO, "\te.g. -6mchops 16\n");

	log_msg(LOG_SEV_INFO, "-6mcloop - loop multicast to localhost.\n");

	log_msg(LOG_SEV_INFO, "-6mcoutif <ifname> - multicast output"
		" interface.\n");
	log_msg(LOG_SEV_INFO, "\te.g. -6mcoutif eth0\n");

	log_msg(LOG_SEV_INFO, "\nsignals:\n");

	log_msg(LOG_SEV_INFO, "SIGUSR1 - log current UDP datagram rx and tx "
		"stats.\n");
	log_msg(LOG_SEV_INFO, "SIGUSR2 - log program parameters.\n");

	log_debug_med("%s() exit\n", __func__);

}


void log_prog_license(void)
{


	log_debug_med("%s() entry\n", __func__);

	log_msg(LOG_SEV_INFO, "\nreplicate IPv4 or IPv6 UDP datagrams to "
		"IPv4 and/or IPv6 destinations.\n");

	log_msg(LOG_SEV_INFO, "\n");

	log_msg(LOG_SEV_INFO, "Copyright (C) 2011 Mark Smith ");
	log_msg(LOG_SEV_INFO, "<markzzzsmith@yahoo.com.au>\n");

	log_msg(LOG_SEV_INFO, "\n");

	log_msg(LOG_SEV_INFO, "This program is free software; you can "
		"redistribute it and/or\n");

	log_msg(LOG_SEV_INFO, "modify it under the terms of the GNU General "
		"Public License.\n");

	log_msg(LOG_SEV_INFO, "as published by the Free Software Foundation; "
		"Version 2 of the\n");

	log_msg(LOG_SEV_INFO, "License.\n");

	log_msg(LOG_SEV_INFO, "\n");

	log_msg(LOG_SEV_INFO, "This program is distributed in the hope that "
		"it will be useful,\n");

	log_msg(LOG_SEV_INFO, "but WITHOUT ANY WARRANTY; without even the "
		"implied warranty of\n");

	log_msg(LOG_SEV_INFO, "MERCHANTABILITY or FITNESS FOR A PARTICULAR "
		"PURPOSE.  See the\n");

	log_msg(LOG_SEV_INFO, "GNU General Public License for more details.\n");

	log_msg(LOG_SEV_INFO, "\n");

	log_msg(LOG_SEV_INFO, "You should have received a copy of the GNU "
		"General Public License\n");

	log_msg(LOG_SEV_INFO, "along with this program; if not, write to "
		"the Free Software\n");

	log_msg(LOG_SEV_INFO, "Foundation, Inc., 51 Franklin Street, "
		"Fifth Floor, Boston, MA  02110-1301, USA.\n");

	log_debug_med("%s() exit\n", __func__);

}


void get_prog_parms(int argc, char *argv[],
	       struct program_parameters *prog_parms,
	       char err_str[],
	       const unsigned int err_str_size)
{
	struct program_options prog_opts;
	enum VALIDATE_PROG_OPTS vpo;
	int vpo_values_ret = 0;


	log_debug_med("%s() entry\n", __func__);

	init_prog_opts(&prog_opts);

	get_prog_opts_cmdline(argc, argv, &prog_opts);

	vpo = validate_prog_opts(&prog_opts);

	switch (vpo) {
	case VPO_HELP:
		prog_parms->rc_mode = RCMODE_HELP;
		break;
	case VPO_LICENSE:
		prog_parms->rc_mode = RCMODE_LICENSE;
		break;
	case VPO_ERR_UNKNOWN_OPT:
		prog_parms->rc_mode = RCMODE_ERROR;
		log_opt_error(OE_UNKNOWN_OPT, NULL);
		break;
	case VPO_ERR_NO_SRC_ADDR:
		prog_parms->rc_mode = RCMODE_ERROR;
		log_opt_error(OE_NO_SRC_ADDR, NULL);
		break;
	case VPO_ERR_MULTI_SRC_ADDRS:
		prog_parms->rc_mode = RCMODE_ERROR;
		log_opt_error(OE_MULTI_SRC_ADDRS, NULL);
		break;
	case VPO_ERR_NO_DST_ADDRS:
		prog_parms->rc_mode = RCMODE_ERROR;
		log_opt_error(OE_NO_DST_ADDRS, NULL);
		break;
	case VPO_MODE_INETINETINET6:
		prog_parms->rc_mode = RCMODE_INET_TO_INET_INET6;
		vpo_values_ret = validate_prog_opts_values(&prog_opts,
							   prog_parms,
							   err_str,
							   err_str_size);		
		if (vpo_values_ret == -1) {
			prog_parms->rc_mode = RCMODE_ERROR;
		}
		break;
	case VPO_MODE_INETINET:
		prog_parms->rc_mode = RCMODE_INET_TO_INET;
		vpo_values_ret = validate_prog_opts_values(&prog_opts,
							   prog_parms,
							   err_str,
							   err_str_size);		
		if (vpo_values_ret == -1) {
			prog_parms->rc_mode = RCMODE_ERROR;
		}
		break;
	case VPO_MODE_INETINET6:
		prog_parms->rc_mode = RCMODE_INET_TO_INET6;
		vpo_values_ret = validate_prog_opts_values(&prog_opts,
							   prog_parms,
							   err_str,
							   err_str_size);		
		if (vpo_values_ret == -1) {
			prog_parms->rc_mode = RCMODE_ERROR;
		}
		break;
	case VPO_MODE_INET6INETINET6:
		prog_parms->rc_mode = RCMODE_INET6_TO_INET_INET6;
		vpo_values_ret = validate_prog_opts_values(&prog_opts,
							   prog_parms,
							   err_str,
							   err_str_size);		
		if (vpo_values_ret == -1) {
			prog_parms->rc_mode = RCMODE_ERROR;
		}
		break;
	case VPO_MODE_INET6INET:
		prog_parms->rc_mode = RCMODE_INET6_TO_INET;
		vpo_values_ret = validate_prog_opts_values(&prog_opts,
							   prog_parms,
							   err_str,
							   err_str_size);		
		if (vpo_values_ret == -1) {
			prog_parms->rc_mode = RCMODE_ERROR;
		}
		break;
	case VPO_MODE_INET6INET6:
		prog_parms->rc_mode = RCMODE_INET6_TO_INET6;
		vpo_values_ret = validate_prog_opts_values(&prog_opts,
							   prog_parms,
							   err_str,
							   err_str_size);		
		if (vpo_values_ret == -1) {
			prog_parms->rc_mode = RCMODE_ERROR;
		}
		break;
	case VPO_ERR_UNKNOWN:
		prog_parms->rc_mode = RCMODE_ERROR;
		log_opt_error(OE_UNKNOWN_ERROR, NULL);
		break;
	default:
		prog_parms->rc_mode = RCMODE_HELP;
		break;
	
	}

	log_debug_med("%s() exit\n", __func__);

}


void init_prog_opts(struct program_options *prog_opts)
{


	log_debug_med("%s() entry\n", __func__);

	prog_opts->help_set = 0;

	prog_opts->license_set = 0;

	prog_opts->unknown_opt_set = 0;
	prog_opts->unknown_opt_str = NULL;

	prog_opts->no_daemon_set = 0;

	prog_opts->inet_rx_sock_mcgroup_set = 0;
	prog_opts->inet_rx_sock_mcgroup_str = NULL;

	prog_opts->inet_tx_sock_mc_ttl_set = 0;
	prog_opts->inet_tx_sock_mc_ttl_str = NULL;
	prog_opts->inet_tx_sock_mc_loop_set = 0;
	prog_opts->inet_tx_sock_out_intf_set = 0;
	prog_opts->inet_tx_sock_out_intf_str = NULL;
	prog_opts->inet_tx_sock_dests_set = 0;
	prog_opts->inet_tx_sock_dests_str = NULL;

	prog_opts->inet6_rx_sock_mcgroup_set = 0;
	prog_opts->inet6_rx_sock_mcgroup_str = NULL;

	prog_opts->inet6_tx_sock_mc_hops_set = 0;
	prog_opts->inet6_tx_sock_mc_hops_str = NULL;
	prog_opts->inet6_tx_sock_mc_loop_set = 0;
	prog_opts->inet6_tx_sock_out_intf_set = 0;
	prog_opts->inet6_tx_sock_out_intf_str = NULL;
	prog_opts->inet6_tx_sock_dests_set = 0;
	prog_opts->inet6_tx_sock_dests_str = NULL;
	
	log_debug_med("%s() exit\n", __func__);

}


void init_prog_parms(struct program_parameters *prog_parms)
{


	log_debug_med("%s() entry\n", __func__);

	prog_parms->rc_mode = RCMODE_UNKNOWN;

	prog_parms->become_daemon = 1;

	prog_parms->inet_rx_sock_parms.rx_addr.s_addr = ntohl(INADDR_NONE);
	prog_parms->inet_rx_sock_parms.port = 0;
	prog_parms->inet_rx_sock_parms.in_intf_addr.s_addr = ntohl(INADDR_ANY);

	prog_parms->inet_tx_sock_parms.mc_ttl = 1;
	prog_parms->inet_tx_sock_parms.mc_loop = 0;
	prog_parms->inet_tx_sock_parms.out_intf_addr.s_addr = ntohl(INADDR_ANY);
	prog_parms->inet_tx_sock_parms.dests = NULL;
	prog_parms->inet_tx_sock_parms.dests_num = 0;
	prog_parms->inet_tx_sock_parms.mc_dests_num = 0;

	memcpy(&prog_parms->inet6_rx_sock_parms.rx_addr, &in6addr_any,
		sizeof(in6addr_any));
	prog_parms->inet6_rx_sock_parms.port = 0;
	prog_parms->inet6_rx_sock_parms.in_intf_idx = 0;

	prog_parms->inet6_tx_sock_parms.mc_hops = 1;
	prog_parms->inet6_tx_sock_parms.mc_loop = 0;
	prog_parms->inet6_tx_sock_parms.out_intf_idx = 0;
	prog_parms->inet6_tx_sock_parms.dests = NULL;
	prog_parms->inet6_tx_sock_parms.dests_num = 0;
	prog_parms->inet6_tx_sock_parms.mc_dests_num = 0;

	log_debug_med("%s() exit\n", __func__);

}


void get_prog_opts_cmdline(int argc, char *argv[],
			   struct program_options *prog_opts)
{
	enum CMDLINE_OPTS {
		CMDLINE_OPT_HELP = 1,
		CMDLINE_OPT_LICENSE,
		CMDLINE_OPT_NODAEMON,
		CMDLINE_OPT_4IN,
		CMDLINE_OPT_4MCTTL,
		CMDLINE_OPT_4MCLOOP,
		CMDLINE_OPT_4MCOUTIF,
		CMDLINE_OPT_4DSTS,
		CMDLINE_OPT_6IN,
		CMDLINE_OPT_6MCHOPS,
		CMDLINE_OPT_6MCLOOP,
		CMDLINE_OPT_6MCOUTIF,
		CMDLINE_OPT_6DSTS,
	};
	struct option cmdline_opts[] = {
		{"help", no_argument, NULL, CMDLINE_OPT_HELP},
		{"license", no_argument, NULL, CMDLINE_OPT_LICENSE},
		{"nodaemon", no_argument, NULL, CMDLINE_OPT_NODAEMON},
		{"4in", required_argument, NULL, CMDLINE_OPT_4IN},
		{"4mcttl", required_argument, NULL, CMDLINE_OPT_4MCTTL},
		{"4mcloop", no_argument, NULL, CMDLINE_OPT_4MCLOOP},
		{"4mcoutif", required_argument, NULL, CMDLINE_OPT_4MCOUTIF},
		{"4out", required_argument, NULL, CMDLINE_OPT_4DSTS},
		{"6in", required_argument, NULL, CMDLINE_OPT_6IN},
		{"6mchops", required_argument, NULL, CMDLINE_OPT_6MCHOPS},
		{"6mcloop", no_argument, NULL, CMDLINE_OPT_6MCLOOP},
		{"6mcoutif", required_argument, NULL, CMDLINE_OPT_6MCOUTIF},
		{"6out", required_argument, NULL, CMDLINE_OPT_6DSTS},
		{0, 0, 0, 0}
	};
	enum CMDLINE_OPTS ret;


	log_debug_med("%s() entry\n", __func__);

	opterr = 0;

	ret = getopt_long_only(argc, argv, ":", cmdline_opts, NULL);
	log_debug_low("%s: getopt_long_only() = %d, %c\n", __func__, ret, ret);
	while ((ret != -1) && (!prog_opts->help_set)
		&& (!prog_opts->license_set) && (!prog_opts->unknown_opt_set)) {
		switch (ret) {
		case CMDLINE_OPT_HELP:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_HELP\n", __func__);
			prog_opts->help_set = 1;
			break;
		case CMDLINE_OPT_LICENSE:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_LICENSE\n", __func__);
			prog_opts->license_set = 1;
			break;
		case CMDLINE_OPT_NODAEMON:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_NODAEMON\n", __func__);
			prog_opts->no_daemon_set = 1;
			break;
		case CMDLINE_OPT_4IN:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_4IN\n", __func__);
			prog_opts->inet_rx_sock_mcgroup_set = 1;
			prog_opts->inet_rx_sock_mcgroup_str = optarg;
			break;
		case CMDLINE_OPT_4MCTTL:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_4TTL\n", __func__);
			prog_opts->inet_tx_sock_mc_ttl_set = 1;
			prog_opts->inet_tx_sock_mc_ttl_str = optarg;
			break;
		case CMDLINE_OPT_4MCLOOP:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_4MCLOOP\n", __func__);
			prog_opts->inet_tx_sock_mc_loop_set = 1;
			break;
		case CMDLINE_OPT_4MCOUTIF:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_4MCOUTIF\n", __func__);
			prog_opts->inet_tx_sock_out_intf_set = 1;
			prog_opts->inet_tx_sock_out_intf_str = optarg;
			break;
		case CMDLINE_OPT_4DSTS:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_4DSTS\n", __func__);
			prog_opts->inet_tx_sock_dests_set = 1;
			prog_opts->inet_tx_sock_dests_str = optarg;
			break;
		case CMDLINE_OPT_6IN:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_6IN\n", __func__);
			prog_opts->inet6_rx_sock_mcgroup_set = 1;
			prog_opts->inet6_rx_sock_mcgroup_str = optarg;
			break;
		case CMDLINE_OPT_6MCHOPS:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_6MCHOPS\n", __func__);
			prog_opts->inet6_tx_sock_mc_hops_set = 1;
			prog_opts->inet6_tx_sock_mc_hops_str = optarg;
			break;
		case CMDLINE_OPT_6MCLOOP:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_6MCLOOP\n", __func__);
			prog_opts->inet6_tx_sock_mc_loop_set = 1;
			break;
		case CMDLINE_OPT_6MCOUTIF:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_6MCOUTIF\n", __func__);
			prog_opts->inet6_tx_sock_out_intf_set = 1;
			prog_opts->inet6_tx_sock_out_intf_str = optarg;
			break;
		case CMDLINE_OPT_6DSTS:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_6DSTS\n", __func__);
			prog_opts->inet6_tx_sock_dests_set = 1;
			prog_opts->inet6_tx_sock_dests_str = optarg;
			break;
		default:
			log_debug_low("%s: getopt_long_only() = "
				"unknown option\n", __func__);
			prog_opts->unknown_opt_set = 1;
			prog_opts->unknown_opt_str = optarg;
			break;
		}

		ret = getopt_long_only(argc, argv, "", cmdline_opts, NULL);
		log_debug_low("%s: getopt_long_only() = %d, %c\n", __func__,
			ret, ret);
	}

	log_debug_med("%s() exit\n", __func__);

}


enum VALIDATE_PROG_OPTS validate_prog_opts(
				const struct program_options *prog_opts)
{


	log_debug_med("%s() entry\n", __func__);

	if (prog_opts->help_set) {
		log_debug_low("%s() return VPO_HELP\n", __func__);
		log_debug_med("%s() exit\n", __func__);
		return VPO_HELP;
	}

	if (prog_opts->license_set) {
		log_debug_low("%s() return VPO_LICENSE\n", __func__);
		log_debug_med("%s() exit\n", __func__);
		return VPO_LICENSE;
	}
	
	if (prog_opts->unknown_opt_set) {
		log_debug_low("%s() return VPO_ERR_UNKNOWN_OPT\n", __func__);
		log_debug_med("%s() exit\n", __func__);
		return VPO_ERR_UNKNOWN_OPT;
	}

	if (!prog_opts->inet_rx_sock_mcgroup_set &&
				!prog_opts->inet6_rx_sock_mcgroup_set) {
		log_debug_low("%s() return VPO_ERR_NO_SRC_ADDR\n", __func__);
		log_debug_med("%s() exit\n", __func__);
		return VPO_ERR_NO_SRC_ADDR;
	}

	if (prog_opts->inet_rx_sock_mcgroup_set &&
				prog_opts->inet6_rx_sock_mcgroup_set) {
		log_debug_low("%s() return VPO_ERR_MULTI_SRC_ADDRS\n", __func__);
		log_debug_med("%s() exit\n", __func__);
		return VPO_ERR_MULTI_SRC_ADDRS;
	}


	if (!prog_opts->inet_tx_sock_dests_set &&
				!prog_opts->inet6_tx_sock_dests_set) {
		log_debug_low("%s() return VPO_ERR_NO_DST_ADDRS\n", __func__);
		log_debug_med("%s() exit\n", __func__);
		return VPO_ERR_NO_DST_ADDRS;
	}

	if (prog_opts->inet_rx_sock_mcgroup_set) {
		if (prog_opts->inet_tx_sock_dests_set &&
                                prog_opts->inet6_tx_sock_dests_set) {
			log_debug_low("%s() return VPO_MODE_INETINETINET6\n",
				__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPO_MODE_INETINETINET6;
		} else if (prog_opts->inet_tx_sock_dests_set) {
			log_debug_low("%s() return VPO_MODE_INETINET\n",
				__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPO_MODE_INETINET;	
		} else if (prog_opts->inet6_tx_sock_dests_set) {
			log_debug_low("%s() return VPO_MODE_INETINET6\n",
				__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPO_MODE_INETINET6;
		}
	}

	if (prog_opts->inet6_rx_sock_mcgroup_set) {
		if (prog_opts->inet_tx_sock_dests_set &&
                                prog_opts->inet6_tx_sock_dests_set) {
			log_debug_low("%s() return VPO_MODE_INET6INETINET6\n",
				__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPO_MODE_INET6INETINET6;
		} else if (prog_opts->inet_tx_sock_dests_set) {
			log_debug_low("%s() return VPO_MODE_INET6INET\n",
				__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPO_MODE_INET6INET;	
		} else if (prog_opts->inet6_tx_sock_dests_set) {
			log_debug_low("%s() return VPO_MODE_INET6INET6\n",
				__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPO_MODE_INET6INET6;
		}
	}

	log_debug_low("%s() return VPO_ERR_UNKNOWN\n", __func__);
	log_debug_med("%s() exit\n", __func__);

	return VPO_ERR_UNKNOWN;

}


enum VALIDATE_PROG_OPTS_VALS validate_prog_opts_vals(
				const struct program_options *prog_opts,
				struct program_parameters *prog_parms,
				char *err_str_parm,
				const unsigned int err_str_size)
{
	int ret;
	struct in_addr in_intf_addr;	
	enum inetaddr_errors aip_ptoh_err;
	int tx_ttl;
	int mc_hops;
	unsigned int out_intf_idx;


	log_debug_med("%s() entry\n", __func__);

	if (prog_opts->no_daemon_set) {
		log_debug_low("%s() prog_opts->no_daemon_set\n", __func__);
		prog_parms->become_daemon = 0;
	}

	if (prog_opts->inet_rx_sock_mcgroup_set) {
		log_debug_low("%s() prog_opts->inet_rx_sock_mcgroup_set\n",
								__func__);
		ret = aip_ptoh_inet(prog_opts->inet_rx_sock_mcgroup_str,
			&prog_parms->inet_rx_sock_parms.rx_addr,
			&in_intf_addr,
			&prog_parms->inet_rx_sock_parms.port,
			&aip_ptoh_err);
		if (ret == -1) {
			switch (aip_ptoh_err) {
			case INETADDR_ERR_ADDR:
				log_debug_low("%s() return "
					"VPOV_ERR_SRC_ADDR\n",
					__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_SRC_ADDR;
				break;
			case INETADDR_ERR_IFADDR:
				log_debug_low("%s() return "
					"VPOV_ERR_IF_ADDR\n",
					__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_IF_ADDR;
				break;
			case INETADDR_ERR_PORT:
				log_debug_low("%s() return "
					"VPOV_ERR_SRC_PORT\n",
					__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_SRC_PORT;
				break;
			default:
				log_debug_low("%s() return "
					"VPOV_ERR_UNKNOWN\n",
					__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_UNKNOWN;
				break;
			}
		} else {
			if (in_intf_addr.s_addr != ntohl(INADDR_NONE)) {
				prog_parms->inet_rx_sock_parms.in_intf_addr =
					in_intf_addr;
			}
			if (prog_parms->inet_rx_sock_parms.port == 0) {
				log_debug_low("%s() return "
					"VPOV_ERR_SRC_PORT\n", __func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_SRC_PORT;
			}
		}
	}

	if (prog_opts->inet6_rx_sock_mcgroup_set) {
		log_debug_low("%s() prog_opts->inet6_rx_sock_mcgroup_set\n",
								__func__);
		ret = aip_ptoh_inet6(prog_opts->inet6_rx_sock_mcgroup_str,
			&prog_parms->inet6_rx_sock_parms.rx_addr,
			&prog_parms->inet6_rx_sock_parms.in_intf_idx,
			&prog_parms->inet6_rx_sock_parms.port,
			&aip_ptoh_err);
		if (ret == -1) {
			switch (aip_ptoh_err) {
			case INETADDR_ERR_ADDR:
				log_debug_low("%s() return "
					"VPOV_ERR_SRC_ADDR\n",
					__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_SRC_ADDR;
				break;
			case INETADDR_ERR_PORT:
				log_debug_low("%s() return "
					"VPOV_ERR_SRC_PORT\n",
					__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_SRC_PORT;
				break;
			default:
				log_debug_low("%s() return "
					"VPOV_ERR_UNKNOWN\n",
					__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_UNKNOWN;
				break;
			}
		} else {
			if (prog_parms->inet6_rx_sock_parms.port == 0) {
				log_debug_low("%s() return "
					"VPOV_ERR_SRC_PORT\n", __func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_SRC_PORT;
			}
		}
	}

	if (prog_opts->inet_tx_sock_dests_set) {
		log_debug_low("%s() prog_opts->inet_tx_sock_dests_set\n",
								__func__);
		ret = ap_pton_inet_csv(
				prog_opts->inet_tx_sock_dests_str,
				NULL, 0, 0, 0, &aip_ptoh_err, err_str_parm, 
				err_str_size);
		log_debug_low("%s() first ap_pton_inet_csv() call = %d\n",
			__func__, ret);
		if (ret == -1) {
			log_debug_low("%s() return VPOV_ERR_INET_DST_ADDR\n",
					__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPOV_ERR_INET_DST_ADDR;
		} else {
			prog_parms->inet_tx_sock_parms.dests =
				(struct sockaddr_in *)
				malloc((ret + 1) * sizeof(struct sockaddr_in));
			if (prog_parms->inet_tx_sock_parms.dests == NULL) {
				log_debug_low("%s() return", __func__);
				log_debug_low(" VPOV_ERR_MEMORY\n");
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_MEMORY;
			}
			ret = ap_pton_inet_csv(
				prog_opts->inet_tx_sock_dests_str,
				prog_parms->inet_tx_sock_parms.dests,
				0, 1, 1, NULL, NULL, 0);
			prog_parms->inet_tx_sock_parms.dests_num = ret;
			prog_parms->inet_tx_sock_parms.mc_dests_num = 
				num_inet_mcaddrs(
					prog_parms->inet_tx_sock_parms.dests,
					prog_parms->
						inet_tx_sock_parms.dests_num);
			log_debug_low("%s() num_inet_mcaddrs = %d\n", __func__,
				prog_parms->inet_tx_sock_parms.mc_dests_num);
		}

		if (prog_opts->inet_tx_sock_mc_ttl_set) {
			log_debug_low("%s() prog_opts->", __func__);
			log_debug_low("inet_tx_sock_mc_ttl_set\n");
			tx_ttl = atoi(prog_opts->inet_tx_sock_mc_ttl_str);
			if ((tx_ttl < 0) || (tx_ttl > 255)) {
				log_debug_low("%s() return "
					"VPOV_ERR_INET_TX_MCTTL_RANGE\n",
						__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_INET_TX_MCTTL_RANGE;
			} else {
				prog_parms->inet_tx_sock_parms.mc_ttl =
									tx_ttl;
			}
		}

		if (prog_opts->inet_tx_sock_mc_loop_set) {
			log_debug_low("%s() prog_opts->", __func__);
			log_debug_low("inet_tx_mc_sock_mc_loop_set\n");
			prog_parms->inet_tx_sock_parms.mc_loop = 1;
		}

		if (prog_opts->inet_tx_sock_out_intf_set) {
			log_debug_low("%s() prog_opts->", __func__);
			log_debug_low("inet_tx_sock_out_intf_set\n");
			ret = inet_if_addr(prog_opts->inet_tx_sock_out_intf_str,
				&prog_parms->inet_tx_sock_parms.out_intf_addr);	
			if (ret == -1) {
				log_debug_low("%s() return "
					"VPOV_ERR_INET_OUT_INTF\n",
						__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_INET_OUT_INTF;
			}
		}
	}

	if (prog_opts->inet6_tx_sock_dests_set) {
		log_debug_low("%s() prog_opts->inet6_tx_sock_dests_set\n",
								__func__);

		ret = ap_pton_inet6_csv(
				prog_opts->inet6_tx_sock_dests_str,
				NULL, 0, 0, 0, &aip_ptoh_err, err_str_parm, 
				err_str_size);
		log_debug_low("%s() first ap_pton_inet6_csv() call = %d\n",
			__func__, ret);
		if (ret == -1) {
			log_debug_low("%s() return VPOV_ERR_INET6_DST_ADDR\n",
					__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPOV_ERR_INET6_DST_ADDR;
		} else {
			prog_parms->inet6_tx_sock_parms.dests =
				(struct sockaddr_in6 *)
				malloc((ret + 1) * sizeof(struct sockaddr_in6));
			if (prog_parms->inet6_tx_sock_parms.dests == NULL) {
				log_debug_low("%s() return", __func__);
				log_debug_low(" VPOV_ERR_MEMORY\n");
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_MEMORY;
			}
			ret = ap_pton_inet6_csv(
				prog_opts->inet6_tx_sock_dests_str,
				prog_parms->inet6_tx_sock_parms.dests,
				0, 1, 1, NULL, NULL, 0);
			prog_parms->inet6_tx_sock_parms.dests_num = ret;
			prog_parms->inet6_tx_sock_parms.mc_dests_num = 
				num_inet6_mcaddrs(
					prog_parms->inet6_tx_sock_parms.dests,
					prog_parms->
						inet6_tx_sock_parms.dests_num);
			log_debug_low("%s() num_inet6_mcaddrs = %d\n", __func__,
				prog_parms->inet6_tx_sock_parms.mc_dests_num);
		}

		if (prog_opts->inet6_tx_sock_mc_hops_set) {
			mc_hops = atoi(prog_opts->inet6_tx_sock_mc_hops_str);
			if ((mc_hops < 0) || (mc_hops > 255)) {
				log_debug_low("%s() return "
					"VPOV_ERR_INET6_TX_HOPS_RANGE\n",
						__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_INET6_TX_HOPS_RANGE;
			} else {
				prog_parms->inet6_tx_sock_parms.mc_hops =
								mc_hops;
			}
		}

		if (prog_opts->inet6_tx_sock_mc_loop_set) {
			log_debug_low("%s() prog_opts->", __func__);
			log_debug_low("inet6_tx_sock_mc_loop_set\n");
			prog_parms->inet6_tx_sock_parms.mc_loop = 1;
		}

		if (prog_opts->inet6_tx_sock_out_intf_set) {
			out_intf_idx = if_nametoindex(prog_opts->
						inet6_tx_sock_out_intf_str);
			if (out_intf_idx == 0) {
				log_debug_low("%s() return "
					"VPOV_ERR_INET6_OUT_INTF\n",
						__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_INET6_OUT_INTF;
			}
			prog_parms->inet6_tx_sock_parms.out_intf_idx =
								 out_intf_idx;
		}
	}

	log_debug_low("%s() return VPOV_OPTS_VALS_VALID\n", __func__);
	log_debug_med("%s() exit\n", __func__);

	return VPOV_OPTS_VALS_VALID;

}


int validate_prog_opts_values(const struct program_options *prog_opts,
			      struct program_parameters *prog_parms,
			      char *err_str_parm,
			      const unsigned int err_str_size)
{
	enum VALIDATE_PROG_OPTS_VALS vpov;
	int ret = -1;


	vpov = validate_prog_opts_vals(prog_opts, prog_parms,
						err_str_parm, err_str_size);
	switch (vpov) {
	case VPOV_ERR_SRC_ADDR:
		log_opt_error(OE_SRC_GRP_ADDR, NULL);
		break;
	case VPOV_ERR_IF_ADDR:
		log_opt_error(OE_IF_ADDR, NULL);
		break;
	case VPOV_ERR_SRC_PORT:
		log_opt_error(OE_SRC_PORT, NULL);
		break;
	case VPOV_ERR_DST_PORT:
		log_opt_error(OE_DST_PORT, NULL);
		break;
	case VPOV_ERR_INET_DST_ADDR:
		log_opt_error(OE_INET_DST_ADDR, NULL);
		break;
	case VPOV_ERR_INET_TX_MCTTL_RANGE:
		log_opt_error(OE_INET_TX_MCTTL_RANGE, NULL);
		break;
	case VPOV_ERR_INET_OUT_INTF:
		log_opt_error(OE_INET_OUT_INTF, NULL);
		break;
	case VPOV_ERR_INET6_OUT_INTF:
		log_opt_error(OE_INET6_OUT_INTF, NULL);
		break;
	case VPOV_ERR_INET6_DST_ADDR:
		log_opt_error(OE_INET6_DST_ADDR, NULL);
		break;
	case VPOV_ERR_INET6_TX_HOPS_RANGE:
		log_opt_error(OE_INET6_TX_HOPS_RANGE, NULL);
		break;
	case VPOV_ERR_MEMORY:
		log_opt_error(OE_MEMORY_ERROR, NULL);
		break;
	case VPOV_ERR_UNKNOWN:
		log_opt_error(OE_UNKNOWN_ERROR, NULL);
		break;
	case VPOV_OPTS_VALS_VALID:
		ret = 0;
		break;
	default:
		break;
	}

	return ret;

}


void log_prog_banner(void)
{


	log_debug_med("%s() entry\n", __func__);

	log_msg(LOG_SEV_INFO, "%s v%1.1f\n", program_name, replicast_version);

	log_debug_med("%s() exit\n", __func__);

}


void log_prog_parms(const struct program_parameters *prog_parms)
{


	log_debug_med("%s() entry\n", __func__);

	switch(prog_parms->rc_mode) {
	case RCMODE_INET_TO_INET:
		log_inet_rx_sock_parms(&prog_parms->inet_rx_sock_parms);
		log_inet_tx_sock_parms(&prog_parms->inet_tx_sock_parms);
		break;
	case RCMODE_INET_TO_INET6:
		log_inet_rx_sock_parms(&prog_parms->inet_rx_sock_parms);
		log_inet6_tx_sock_parms(&prog_parms->inet6_tx_sock_parms);
		break;
	case RCMODE_INET_TO_INET_INET6:
		log_inet_rx_sock_parms(&prog_parms->inet_rx_sock_parms);
		log_inet_tx_sock_parms(&prog_parms->inet_tx_sock_parms);
		log_inet6_tx_sock_parms(&prog_parms->inet6_tx_sock_parms);
		break;
	case RCMODE_INET6_TO_INET6:
		log_inet6_rx_sock_parms(&prog_parms->inet6_rx_sock_parms);
		log_inet6_tx_sock_parms(&prog_parms->inet6_tx_sock_parms);
		break;
	case RCMODE_INET6_TO_INET:
		log_inet6_rx_sock_parms(&prog_parms->inet6_rx_sock_parms);
		log_inet_tx_sock_parms(&prog_parms->inet_tx_sock_parms);
		break;
	case RCMODE_INET6_TO_INET_INET6:
		log_inet6_rx_sock_parms(&prog_parms->inet6_rx_sock_parms);
		log_inet_tx_sock_parms(&prog_parms->inet_tx_sock_parms);
		log_inet6_tx_sock_parms(&prog_parms->inet6_tx_sock_parms);
		break;
	default:
		break;
	}


	log_debug_med("%s() exit\n", __func__);

}


void log_inet_rx_sock_parms(const struct inet_rx_sock_params *inet_rx_parms)
{
	char aip_str[AIP_STR_INET_MAX_LEN + 1];
	const unsigned int aip_str_size = AIP_STR_INET_MAX_LEN + 1;


	log_debug_med("%s() entry\n", __func__);

	log_msg(LOG_SEV_INFO, "inet rx src: ");

	aip_htop_inet(&inet_rx_parms->rx_addr, &inet_rx_parms->in_intf_addr,
		inet_rx_parms->port, aip_str, aip_str_size);

	log_msg(LOG_SEV_INFO, "%s\n", aip_str);

	log_debug_med("%s() exit\n", __func__);

}


void log_inet6_rx_sock_parms(const struct inet6_rx_sock_params
							*inet6_rx_parms)
{
	char aip_str[AIP_STR_INET6_MAX_LEN + 1];
	const unsigned int aip_str_size = AIP_STR_INET6_MAX_LEN + 1;


	log_debug_med("%s() entry\n", __func__);

	log_msg(LOG_SEV_INFO, "inet6 rx src: ");

	aip_htop_inet6(&inet6_rx_parms->rx_addr, inet6_rx_parms->in_intf_idx,
		inet6_rx_parms->port, aip_str, aip_str_size);

	log_msg(LOG_SEV_INFO, "%s\n", aip_str);

	log_debug_med("%s() exit\n", __func__);

}


void log_inet_tx_sock_parms(const struct inet_tx_sock_params *inet_tx_parms)
{
	const unsigned int ap_str_size = INET_ADDRSTRLEN + 1 + 5 + 1;
	char ap_str[ap_str_size];
	unsigned int dest_num = 0;
	char out_intf_addr_str[INET_ADDRSTRLEN];


	log_debug_med("%s() entry\n", __func__);

	log_msg(LOG_SEV_INFO, "inet tx dsts: ");

	for (dest_num = 0; dest_num < (inet_tx_parms->dests_num - 1);
								dest_num++) {

		ap_htop_inet(&inet_tx_parms->dests[dest_num].sin_addr,
			ntohs(inet_tx_parms->dests[dest_num].sin_port),
			ap_str, ap_str_size);
		log_msg(LOG_SEV_INFO, "%s,", ap_str);
	}

	ap_htop_inet(&inet_tx_parms->dests[dest_num].sin_addr,
		ntohs(inet_tx_parms->dests[dest_num].sin_port),
		ap_str, ap_str_size);
	log_msg(LOG_SEV_INFO, "%s\n", ap_str);

	log_msg(LOG_SEV_INFO, "inet tx opts: ");

	if (inet_tx_parms->out_intf_addr.s_addr == ntohl(INADDR_ANY)) {
		log_msg(LOG_SEV_INFO, "out intf rt table");
	} else {
		inet_ntop(AF_INET, &inet_tx_parms->out_intf_addr,
			out_intf_addr_str, INET_ADDRSTRLEN);
		log_msg(LOG_SEV_INFO, "out intf addr %s", out_intf_addr_str);
	}

	if (inet_tx_parms->mc_loop) {
		log_msg(LOG_SEV_INFO, ", tx loop");
	}

	log_msg(LOG_SEV_INFO, ", mc ttl %d\n", inet_tx_parms->mc_ttl);

	log_debug_med("%s() exit\n", __func__);

}


void log_inet6_tx_sock_parms(const struct inet6_tx_sock_params
								*inet6_tx_parms)
{
	const unsigned int ap_str_size = 1 + INET6_ADDRSTRLEN + 1 + 1 + 5 + 1;
	char ap_str[ap_str_size];
	unsigned int dest_num = 0;
	char out_intf_name[IFNAMSIZ];


	log_debug_med("%s() entry\n", __func__);

	log_msg(LOG_SEV_INFO, "inet6 tx dsts: ");

	for (dest_num = 0; dest_num < (inet6_tx_parms->dests_num - 1);
								dest_num++) {

		ap_htop_inet6(&inet6_tx_parms->dests[dest_num].sin6_addr,
			ntohs(inet6_tx_parms->dests[dest_num].sin6_port),
			ap_str, ap_str_size);
		log_msg(LOG_SEV_INFO, "%s,", ap_str);
	}

	ap_htop_inet6(&inet6_tx_parms->dests[dest_num].sin6_addr,
		ntohs(inet6_tx_parms->dests[dest_num].sin6_port),
		ap_str, ap_str_size);
	log_msg(LOG_SEV_INFO, "%s\n", ap_str);

	log_msg(LOG_SEV_INFO, "inet6 tx opts: ");

	if (inet6_tx_parms->out_intf_idx == 0) {
		log_msg(LOG_SEV_INFO, "out intf rt table");
	} else {
		if_indextoname(inet6_tx_parms->out_intf_idx, out_intf_name);
		log_msg(LOG_SEV_INFO, "out intf %s", out_intf_name);
	}

	if (inet6_tx_parms->mc_loop) {
		log_msg(LOG_SEV_INFO, ", tx loop");
	}

	log_msg(LOG_SEV_INFO, ", mc hops %d\n", inet6_tx_parms->mc_hops);

	log_debug_med("%s() exit\n", __func__);

}


void log_opt_error(enum OPT_ERR option_err,
		   const char *err_str_parm)
{


	log_debug_med("%s() entry\n", __func__);

	log_prog_banner();

	switch (option_err) {
	case OE_UNKNOWN_OPT:
		log_msg(LOG_SEV_ERR, "Unknown option.\n");
		break;
	case OE_NO_SRC_ADDR:
		log_msg(LOG_SEV_ERR, "No incoming address specified.\n");
		break;
	case OE_MULTI_SRC_ADDRS:
		log_msg(LOG_SEV_ERR, "Too many incoming addresses.\n");
		break;
	case OE_NO_DST_ADDRS:
		log_msg(LOG_SEV_ERR, "No outgoing address(es) ");
		log_msg(LOG_SEV_ERR, "specified.\n");
		break;
	case OE_SRC_GRP_ADDR:
		log_msg(LOG_SEV_ERR, "Invalid traffic source.\n");
		break;	
	case OE_IF_ADDR:
		log_msg(LOG_SEV_ERR, "Invalid interface address.\n");
		break;
	case OE_SRC_PORT:
		log_msg(LOG_SEV_ERR, "Invalid or missing source port.\n");
		break;
	case OE_DST_PORT:
		log_msg(LOG_SEV_ERR, "Invalid destination port.\n");
		break;
	case OE_INET_DST_ADDR:
		log_msg(LOG_SEV_ERR, "Invalid IPv4 destination address.\n");
		break;
	case OE_INET_TX_MCTTL_RANGE:
		log_msg(LOG_SEV_ERR, "Invalid multicast IPv4 transmit ");
		log_msg(LOG_SEV_ERR, "time-to-live.\n");
		break;
	case OE_INET_OUT_INTF:
		log_msg(LOG_SEV_ERR, "Invalid IPv4 multicast output");
		log_msg(LOG_SEV_ERR, " interface.\n");
		break;
	case OE_INET6_OUT_INTF:
		log_msg(LOG_SEV_ERR, "Invalid IPv6 multicast output");
		log_msg(LOG_SEV_ERR, " interface.\n");
		break;
	case OE_INET6_DST_ADDR:
		log_msg(LOG_SEV_ERR, "Invalid IPv6 destination address.\n");
		break;
	case OE_INET6_TX_HOPS_RANGE:
		log_msg(LOG_SEV_ERR, "Invalid IPv6 transmit hop-count.\n");
		break;
	case OE_MEMORY_ERROR:
		log_msg(LOG_SEV_ERR, "Fatal memory error during option ");
		log_msg(LOG_SEV_ERR, "parsing.\n");
		break;
	case OE_UNKNOWN_ERROR:
		log_msg(LOG_SEV_ERR, "Unknown option error.\n");
		break;
	default:
		log_msg(LOG_SEV_ERR, "Option error not specified.\n");
		break;
	}

	log_debug_med("%s() exit\n", __func__);

}


void cleanup_prog_parms(struct program_parameters *prog_parms)
{


	log_debug_med("%s() entry\n", __func__);

	if (prog_parms->inet_tx_sock_parms.dests != NULL) {
		free(prog_parms->inet_tx_sock_parms.dests);
		prog_parms->inet_tx_sock_parms.dests = NULL;
	}

	if (prog_parms->inet6_tx_sock_parms.dests != NULL) {
		free(prog_parms->inet6_tx_sock_parms.dests);
		prog_parms->inet6_tx_sock_parms.dests = NULL;
	}

	log_debug_med("%s() exit\n", __func__);

}


void init_sock_fds(struct socket_fds *sock_fds)
{

	sock_fds->inet_in_sock_fd = -1;
	sock_fds->inet6_in_sock_fd = -1;
	sock_fds->inet_out_sock_fd = -1;
	sock_fds->inet6_out_sock_fd = -1;

}


void inet_to_inet_rcast(int *inet_in_sock_fd,
			const struct inet_rx_sock_params *rx_sock_parms,
			int *inet_out_sock_fd,
			const struct inet_tx_sock_params *tx_sock_parms,
			struct packet_counters *pkt_counters)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;
	int txed_pkts;


	log_debug_med("%s() entry\n", __func__);

	*inet_in_sock_fd = open_inet_rx_sock(rx_sock_parms);
	if (*inet_in_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet_out_sock_fd = open_inet_tx_sock(tx_sock_parms);
	if (*inet_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	for ( ;; ) {
		rx_pkt_len = recv(*inet_in_sock_fd, pkt_buf, PKT_BUF_SIZE, 0);
		log_debug_low("%s(): recv() == %d\n", __func__, rx_pkt_len);
		log_debug_low("%s(): errno == %d, %s\n", __func__, errno,
							strerror(errno));
		if (rx_pkt_len > 0) {
			pkt_counters->inet_in_pkts++;
			txed_pkts = inet_tx_rcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms->dests);
			pkt_counters->inet_out_pkts += txed_pkts;
		}
	}

	log_debug_med("%s() exit\n", __func__);

}


void inet_to_inet6_rcast(int *inet_in_sock_fd,
			 const struct inet_rx_sock_params *rx_sock_parms,
			 int *inet6_out_sock_fd,
			 const struct inet6_tx_sock_params *tx_sock_parms,
			 struct packet_counters *pkt_counters)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;
	int txed_pkts;


	log_debug_med("%s() entry\n", __func__);

	*inet_in_sock_fd = open_inet_rx_sock(rx_sock_parms);
	if (*inet_in_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet6_out_sock_fd = open_inet6_tx_sock(tx_sock_parms);
	if (*inet6_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	for ( ;; ) {
		rx_pkt_len = recv(*inet_in_sock_fd, pkt_buf, PKT_BUF_SIZE, 0);
		log_debug_low("%s(): recv() == %d\n", __func__, rx_pkt_len);
		log_debug_low("%s(): errno == %d, %s\n", __func__, errno,
							strerror(errno));
		if (rx_pkt_len > 0) {
			pkt_counters->inet_in_pkts++;
			txed_pkts = inet6_tx_rcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms->dests);
			pkt_counters->inet6_out_pkts += txed_pkts;
		}
	}

	log_debug_med("%s() exit\n", __func__);

}


void inet_to_inet_inet6_rcast(int *inet_in_sock_fd,
			      const struct inet_rx_sock_params *rx_sock_parms,
			      int *inet_out_sock_fd,
			      const struct inet_tx_sock_params
							*inet_tx_sock_parms,
			      int *inet6_out_sock_fd,
			      const struct inet6_tx_sock_params
							*inet6_tx_sock_parms,
			      struct packet_counters *pkt_counters)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;
	int txed_inet_pkts;
	int txed_inet6_pkts;


	log_debug_med("%s() entry\n", __func__);

	*inet_in_sock_fd = open_inet_rx_sock(rx_sock_parms);
	if (*inet_in_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet_out_sock_fd = open_inet_tx_sock(inet_tx_sock_parms);
	if (*inet_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet6_out_sock_fd = open_inet6_tx_sock(inet6_tx_sock_parms);
	if (*inet6_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	for ( ;; ) {
		rx_pkt_len = recv(*inet_in_sock_fd, pkt_buf, PKT_BUF_SIZE, 0);
		log_debug_low("%s(): recv() == %d\n", __func__, rx_pkt_len);
		log_debug_low("%s(): errno == %d, %s\n", __func__, errno,
							strerror(errno));
		if (rx_pkt_len > 0) {
			pkt_counters->inet_in_pkts++;
			txed_inet_pkts = inet_tx_rcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				inet_tx_sock_parms->dests);
			pkt_counters->inet_out_pkts += txed_inet_pkts;
			txed_inet6_pkts = inet6_tx_rcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				inet6_tx_sock_parms->dests);
			pkt_counters->inet6_out_pkts += txed_inet6_pkts;
		}
	}

	log_debug_med("%s() exit\n", __func__);

}


void inet6_to_inet6_rcast(int *inet6_in_sock_fd,
			  const struct inet6_rx_sock_params *rx_sock_parms,
			  int *inet6_out_sock_fd,
			  const struct inet6_tx_sock_params *tx_sock_parms,
			  struct packet_counters *pkt_counters)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;
	int txed_pkts;


	log_debug_med("%s() entry\n", __func__);

	*inet6_in_sock_fd = open_inet6_rx_sock(rx_sock_parms);
	if (*inet6_in_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet6_out_sock_fd = open_inet6_tx_sock(tx_sock_parms);
	if (*inet6_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	for ( ;; ) {
		rx_pkt_len = recv(*inet6_in_sock_fd, pkt_buf, PKT_BUF_SIZE, 0);
		log_debug_low("%s(): recv() == %d\n", __func__, rx_pkt_len);
		log_debug_low("%s(): errno == %d, %s\n", __func__, errno,
							strerror(errno));
		if (rx_pkt_len > 0) {
			pkt_counters->inet6_in_pkts++;
			txed_pkts = inet6_tx_rcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms->dests);
			pkt_counters->inet6_out_pkts += txed_pkts;
		}
	}

	log_debug_med("\t%s() exit\n", __func__);

}


void inet6_to_inet_rcast(int *inet6_in_sock_fd,
			 const struct inet6_rx_sock_params *rx_sock_parms,
			 int *inet_out_sock_fd,
			 const struct inet_tx_sock_params *tx_sock_parms,
			 struct packet_counters *pkt_counters)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;
	int txed_pkts;


	log_debug("%s() entry\n", __func__);

	*inet6_in_sock_fd = open_inet6_rx_sock(rx_sock_parms);
	if (*inet6_in_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet_out_sock_fd = open_inet_tx_sock(tx_sock_parms);
	if (*inet_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	for ( ;; ) {
		rx_pkt_len = recv(*inet6_in_sock_fd, pkt_buf, PKT_BUF_SIZE, 0);
		log_debug_low("%s(): recv() == %d\n", __func__, rx_pkt_len);
		log_debug_low("%s(): errno == %d, %s\n", __func__, errno,
							strerror(errno));
		if (rx_pkt_len > 0) {
			pkt_counters->inet6_in_pkts++;
			txed_pkts = inet_tx_rcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms->dests);
			pkt_counters->inet_out_pkts += txed_pkts;
		}
	}

}

void inet6_to_inet_inet6_rcast(int *inet6_in_sock_fd,
			       const struct inet6_rx_sock_params *rx_sock_parms,
			       int *inet_out_sock_fd,
			       const struct inet_tx_sock_params
							*inet_tx_sock_parms,
			       int *inet6_out_sock_fd,
			       const struct inet6_tx_sock_params
							*inet6_tx_sock_parms,
			       struct packet_counters *pkt_counters)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;
	int txed_inet_pkts;
	int txed_inet6_pkts;


	log_debug("%s() entry\n", __func__);

	*inet6_in_sock_fd = open_inet6_rx_sock(rx_sock_parms);
	if (*inet6_in_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet_out_sock_fd = open_inet_tx_sock(inet_tx_sock_parms);
	if (*inet_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	*inet6_out_sock_fd = open_inet6_tx_sock(inet6_tx_sock_parms);
	if (*inet6_out_sock_fd == -1) {
		exit_errno(__func__, __LINE__, errno);
	}

	for ( ;; ) {
		rx_pkt_len = recv(*inet6_in_sock_fd, pkt_buf, PKT_BUF_SIZE, 0);
		log_debug_low("%s(): recv() == %d\n", __func__, rx_pkt_len);
		log_debug_low("%s(): errno == %d, %s\n", __func__, errno,
							strerror(errno));
		if (rx_pkt_len > 0) {
			pkt_counters->inet6_in_pkts++;
			txed_inet_pkts = inet_tx_rcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				inet_tx_sock_parms->dests);
			pkt_counters->inet_out_pkts += txed_inet_pkts;
			txed_inet6_pkts = inet6_tx_rcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				inet6_tx_sock_parms->dests);
			pkt_counters->inet6_out_pkts += txed_inet6_pkts;
		}
	}

}


void exit_errno(const char *func_name, const unsigned int linenum, int errnum)
{


	log_debug("%s() entry\n", __func__);

	log_msg(LOG_SEV_ERR, "%s():%d error: %s\n", func_name, linenum,
		strerror(errnum));

	log_debug("%s() exit\n", __func__);

	exit(EXIT_FAILURE);

}


void init_packet_counters(struct packet_counters *pkt_counters)
{


	pkt_counters->inet_in_pkts = 0;
	pkt_counters->inet_out_pkts = 0;
	pkt_counters->inet6_in_pkts = 0;
	pkt_counters->inet6_out_pkts = 0;

}


void daemonise(void)
{


	log_debug_med("%s() entry\n", __func__);

	create_child_process();

	create_new_session();

	change_to_rootdir();

	open_syslog_log();

	close_stdfiles();

	log_debug_med("%s() exit\n", __func__);

}


void create_child_process(void)
{
	pid_t pid;


	log_debug_med("%s() entry\n", __func__);

	pid = fork();
	if (pid < 0) {
		exit_errno(__func__, __LINE__, errno);
	} else if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	log_debug_med("%s() exit\n", __func__);

}


void create_new_session(void)
{
	pid_t sid;


	log_debug_med("%s() entry\n", __func__);

	sid = setsid();
	if (sid < 0) {
		exit_errno(__func__, __LINE__, errno);
	}

	log_debug_med("%s() exit\n", __func__);

}


void change_to_rootdir(void)
{


	log_debug_med("%s() entry\n", __func__);

	if ((chdir("/")) < 0) {
		exit_errno(__func__, __LINE__, errno);
	}

	log_debug_med("%s() exit\n", __func__);

}


void open_syslog_log(void)
{
	pid_t daemon_pid;


	log_debug_med("%s() entry\n", __func__);

	daemon_pid = getpid();

	snprintf(syslog_ident, syslog_ident_len, "%s[%d]", program_name,
		daemon_pid);
	syslog_ident[syslog_ident_len - 1] = '\0';

	log_open(LOG_DEST_SYSLOG, syslog_ident, LOG_SYSLOG_DAEMON, NULL, NULL,
		 NULL, NULL);

	log_debug_med("%s() exit\n", __func__);

}


void close_stdfiles(void)
{


	log_debug_med("%s() entry\n", __func__);

	close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);	

	log_debug_med("%s() exit\n", __func__);

}


void install_exit_signal_handlers(void)
{


	log_debug_med("%s() entry\n", __func__);

	install_sigterm_handler(&sigterm_action);

	install_sigint_handler(&sigint_action);

	log_debug_med("%s() exit\n", __func__);

}


void install_usr_signal_handlers(void)
{


	log_debug_med("%s() entry\n", __func__);

	install_sigusr1_handler(&sigusr1_action);

	install_sigusr2_handler(&sigusr2_action);

	log_debug_med("%s() exit\n", __func__);

}


void install_sigterm_handler(struct sigaction *sigterm_action)
{


	log_debug_med("%s() entry\n", __func__);

	sigterm_action->sa_handler = sigterm_handler;

        sigemptyset(&sigterm_action->sa_mask);
	sigaddset(&sigterm_action->sa_mask, SIGINT);
	sigaddset(&sigterm_action->sa_mask, SIGUSR1);
	sigaddset(&sigterm_action->sa_mask, SIGUSR2);

        sigterm_action->sa_flags = 0;

        sigaction(SIGTERM, sigterm_action, NULL);

	log_debug_med("%s() exit\n", __func__);

}


void install_sigint_handler(struct sigaction *sigint_action)
{


	log_debug_med("%s() entry\n", __func__);

	sigint_action->sa_handler = sigint_handler;

        sigemptyset(&sigint_action->sa_mask);
	sigaddset(&sigint_action->sa_mask, SIGTERM);
	sigaddset(&sigint_action->sa_mask, SIGUSR1);
	sigaddset(&sigint_action->sa_mask, SIGUSR2);

        sigint_action->sa_flags = 0;

        sigaction(SIGINT, sigint_action, NULL);

	log_debug_med("%s() exit\n", __func__);

}


void install_sigusr1_handler(struct sigaction *sigusr1_action)
{


	log_debug_med("%s() entry\n", __func__);

	sigusr1_action->sa_handler = sigusr1_handler;

        sigemptyset(&sigusr1_action->sa_mask);
	sigaddset(&sigusr1_action->sa_mask, SIGUSR2);

        sigusr1_action->sa_flags = SA_RESTART;

        sigaction(SIGUSR1, sigusr1_action, NULL);

	log_debug_med("%s() exit\n", __func__);

}


void install_sigusr2_handler(struct sigaction *sigusr2_action)
{


	log_debug_med("%s() entry\n", __func__);

	sigusr2_action->sa_handler = sigusr2_handler;

        sigemptyset(&(sigusr2_action->sa_mask));
	sigaddset(&sigusr2_action->sa_mask, SIGUSR1);

        sigusr2_action->sa_flags = SA_RESTART;

        sigaction(SIGUSR2, sigusr2_action, NULL);

	log_debug_med("%s() exit\n", __func__);

}


void sigterm_handler(int signum)
{


	log_debug_med("%s() entry\n", __func__);

	exit_program();

	log_debug_med("%s() exit\n", __func__);

}


void sigint_handler(int signum)
{


	log_debug_med("%s() entry\n", __func__);

	exit_program();

	log_debug_med("%s() exit\n", __func__);


}


void sigusr1_handler(int signum)
{


	log_debug_med("%s() entry\n", __func__);

	log_packet_counters(prog_parms.rc_mode, &pkt_counters);

	log_debug_med("%s() exit\n", __func__);


}


void sigusr2_handler(int signum)
{


	log_debug_med("%s() entry\n", __func__);

	log_prog_banner();

	log_prog_parms(&prog_parms);

	log_debug_med("%s() exit\n", __func__);


}


int open_inet_rx_sock(const struct inet_rx_sock_params *sock_parms)
{
	int ret;
	int sock_fd;
	int one = 1;
	struct sockaddr_in sa_in_rxaddr;
	struct ip_mreq ip_mcast_req;


	log_debug_med("%s() entry\n", __func__);

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		return -1;
	}

	ret = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &one,
		sizeof(one));
	if (ret == -1) {
		return -1;
	}

	memset(&sa_in_rxaddr, 0, sizeof(sa_in_rxaddr));
	sa_in_rxaddr.sin_family = AF_INET;
	sa_in_rxaddr.sin_addr =  sock_parms->rx_addr;
	sa_in_rxaddr.sin_port = htons(sock_parms->port);
	ret = bind(sock_fd, (struct sockaddr *) &sa_in_rxaddr,
		sizeof(sa_in_rxaddr));
	if (ret == -1) {
		return -1;
	}

	if (IN_MULTICAST(ntohl(sock_parms->rx_addr.s_addr))) {
		ip_mcast_req.imr_multiaddr = sock_parms->rx_addr;
		ip_mcast_req.imr_interface = sock_parms->in_intf_addr;
		ret = setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			&ip_mcast_req, sizeof(ip_mcast_req));
		if (ret == -1) {
			return -1;
		}
	}

	log_debug_med("%s() exit\n", __func__);

	return sock_fd;

}


void close_inet_rx_sock(const int sock_fd)
{


	log_debug_med("%s() entry\n", __func__);

	if (sock_fd != -1) {
		close(sock_fd);
	}

	log_debug_med("%s() exit\n", __func__);

}


int open_inet6_rx_sock(const struct inet6_rx_sock_params *sock_parms)
{
	int ret;
	int sock_fd;
	int one = 1;
	struct sockaddr_in6 sa_in6_rxaddr;
	struct ipv6_mreq ipv6_mcast_req;


	log_debug_med("%s() entry\n", __func__);

	sock_fd = socket(AF_INET6, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		log_debug_low("%s(): socket() == %d\n", __func__, sock_fd);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		log_debug_med("%s() exit\n", __func__);
		return -1;
	}

	ret = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &one,
		sizeof(one));
	if (ret == -1) {
		log_debug_low("%s(): setsockopt(SO_REUSEADDR) == %d\n",
			__func__, ret);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		log_debug_med("%s() exit\n", __func__);
		return -1;
	}

	memset(&sa_in6_rxaddr, 0, sizeof(sa_in6_rxaddr));
	sa_in6_rxaddr.sin6_family = AF_INET6;
	sa_in6_rxaddr.sin6_addr = sock_parms->rx_addr;
	sa_in6_rxaddr.sin6_port = htons(sock_parms->port);
	if (IN6_IS_ADDR_MC_LINKLOCAL(&sa_in6_rxaddr.sin6_addr) ||
	    IN6_IS_ADDR_LINKLOCAL(&sa_in6_rxaddr.sin6_addr) ) {
		sa_in6_rxaddr.sin6_scope_id = sock_parms->in_intf_idx;
	}
	ret = bind(sock_fd, (struct sockaddr *) &sa_in6_rxaddr,
		sizeof(sa_in6_rxaddr));
	if (ret == -1) {
		log_debug_low("%s(): bind() == %d\n", __func__, ret);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		log_debug_med("%s() exit\n", __func__);
		return -1;
	}

	if (IN6_IS_ADDR_MULTICAST(&sock_parms->rx_addr)) {
		ipv6_mcast_req.ipv6mr_multiaddr = sock_parms->rx_addr;
		ipv6_mcast_req.ipv6mr_interface = sock_parms->in_intf_idx;
		ret = setsockopt(sock_fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
			&ipv6_mcast_req, sizeof(ipv6_mcast_req));
		if (ret == -1) {
			log_debug_low("%s(): setsockopt(IPV6_ADD_MEMBERSHIP)",
				__func__);
			log_debug_low(" == %d\n", ret);
			log_debug_low("%s(): errno == %d\n", __func__, errno);
			log_debug_med("%s() exit\n", __func__);
			return -1;
		}
	}

	log_debug_med("%s() exit\n", __func__);

	return sock_fd;

}


void close_inet6_rx_sock(const int sock_fd)
{


	log_debug_med("%s() entry\n", __func__);

	if (sock_fd != -1) {
		close(sock_fd);
	}

	log_debug_med("%s() exit\n", __func__);

}


int open_inet_tx_sock(const struct inet_tx_sock_params *sock_parms)
{
	int sock_fd;
	uint8_t ttl;
	uint8_t loop;
	int ret;


	log_debug_med("%s() entry\n", __func__);

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		return -1;
	}

	if (sock_parms->mc_dests_num > 0) {

		log_debug_low("%s() sock_parms->mc_dests_num = %d\n", __func__,
			sock_parms->mc_dests_num);

		if (sock_parms->mc_ttl > 0) {
			ttl = sock_parms->mc_ttl & 0xff;
			ret = setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_TTL,
				&ttl, sizeof(ttl));	
			if (ret == -1) {
				return -1;
			}
		}

		if (sock_parms->mc_loop == 1) {
			loop = 1;
		} else {
			loop = 0;
		}
		ret = setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop,
			sizeof(loop));
		if (ret == -1) {
			return -1;
		}

		ret = setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_IF,
			&sock_parms->out_intf_addr,
			sizeof(sock_parms->out_intf_addr));
		if (ret == -1) {
			return -1;
		}
 
	}

	log_debug_med("%s() exit\n", __func__);

	return sock_fd;

}


void close_inet_tx_sock(int sock_fd)
{


	log_debug_med("%s() entry\n", __func__);

	if (sock_fd != -1) {
		close(sock_fd);
	}

	log_debug_med("%s() exit\n", __func__);

}


int open_inet6_tx_sock(const struct inet6_tx_sock_params *sock_parms)
{
	int sock_fd;
	int ret;
	int32_t hops;
	uint32_t loop;
	uint32_t out_ifidx;


	log_debug_med("%s() entry\n", __func__);

	sock_fd = socket(AF_INET6, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		return -1;
	}

	if (sock_parms->mc_dests_num > 0) {

		log_debug_low("%s() sock_parms->mc_dests_num = %d\n", __func__,
			sock_parms->mc_dests_num);

		if (sock_parms->mc_hops > 0) {
			hops = sock_parms->mc_hops & 0xff;
			ret = setsockopt(sock_fd, IPPROTO_IPV6,
				IPV6_MULTICAST_HOPS, &hops, sizeof(hops));	
			if (ret == -1) {
				return -1;
			}
		}

		if (sock_parms->mc_loop == 1) {
			loop = 1;
		} else {
			loop = 0;
		}
		ret = setsockopt(sock_fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
			&loop, sizeof(loop));
		if (ret == -1) {
			return -1;
		}

		if (sock_parms->out_intf_idx) {
			out_ifidx = sock_parms->out_intf_idx;
			ret = setsockopt(sock_fd, IPPROTO_IPV6,
				IPV6_MULTICAST_IF, &out_ifidx,
				sizeof(out_ifidx));
			if (ret == -1) {
				return -1;
			}
		}

	}

	log_debug_med("%s() exit\n", __func__);

	return sock_fd;
			
}


void close_inet6_tx_sock(int sock_fd)
{


	log_debug_med("%s() entry\n", __func__);

	if (sock_fd != -1) {
		close(sock_fd);
	}

	log_debug_med("%s() exit\n", __func__);

}


int inet_tx_rcast(const int sock_fd,
		  const void *pkt,
		  const size_t pkt_len,
 		  const struct sockaddr_in inet_dests[])
{
	const struct sockaddr_in *sa_dest;
	unsigned int tx_success = 0;
	ssize_t ret;


	log_debug_med("%s() entry\n", __func__);

	sa_dest = inet_dests;

	while (sa_dest->sin_family == AF_INET) {
		ret = sendto(sock_fd, pkt, pkt_len, 0,
			(struct sockaddr *)sa_dest,
			sizeof(struct sockaddr_in));
		if (ret != -1) {
			tx_success++;
		}
		log_debug_low("%s(): sendto() == %d\n", __func__, ret);
		log_debug_low("%s(): errno == %d\n", __func__, errno);

		sa_dest++;
	}

	log_debug_med("%s() exit\n", __func__);

	return tx_success;

}


int inet6_tx_rcast(const int sock_fd,
		   const void *pkt,
		   const size_t pkt_len,
 		   const struct sockaddr_in6 inet6_dests[])
{
	const struct sockaddr_in6 *sa6_dest;
	unsigned int tx_success = 0;
	ssize_t ret;


	log_debug_med("%s() entry\n", __func__);

	sa6_dest = inet6_dests;

	while (sa6_dest->sin6_family == AF_INET6) {
		ret = sendto(sock_fd, pkt, pkt_len, 0, 	
			(struct sockaddr *)sa6_dest,
			sizeof(struct sockaddr_in6));
		if (ret != -1) {
			tx_success++;
		}
		log_debug_low("%s(): sendto() == %d\n", __func__, ret);
		log_debug_low("%s(): errno == %d\n", __func__, errno);

		sa6_dest++;
	}
	
	log_debug_med("%s() exit\n", __func__);

	return tx_success;

}


void close_sockets(const struct socket_fds *sock_fds)
{


	log_debug_med("%s() entry\n", __func__);

	close_inet_rx_sock(sock_fds->inet_in_sock_fd);
	close_inet6_rx_sock(sock_fds->inet6_in_sock_fd);
	close_inet_tx_sock(sock_fds->inet_out_sock_fd);
	close_inet6_tx_sock(sock_fds->inet6_out_sock_fd);

	log_debug_med("%s() exit\n", __func__);

}


void log_packet_counters(const enum REPLICAST_MODE rc_mode,
			 const struct packet_counters *pkt_counters)
{


	log_debug_med("%s() entry\n", __func__);

	switch (rc_mode) {
	case RCMODE_INET_TO_INET:
		log_msg(LOG_SEV_INFO, "inet pkts in %lld, ",
						pkt_counters->inet_in_pkts);
		log_msg(LOG_SEV_INFO, "inet pkts out %lld\n",
						pkt_counters->inet_out_pkts);
		break;
	case RCMODE_INET_TO_INET6:
		log_msg(LOG_SEV_INFO, "inet pkts in %lld, ",
						pkt_counters->inet_in_pkts);
		log_msg(LOG_SEV_INFO, "inet6 pkts out %lld\n",
						pkt_counters->inet6_out_pkts);
		break;
	case RCMODE_INET_TO_INET_INET6:
		log_msg(LOG_SEV_INFO, "inet pkts in %lld, ",
						pkt_counters->inet_in_pkts);
		log_msg(LOG_SEV_INFO, "inet pkts out %lld, ",
						pkt_counters->inet_out_pkts);
		log_msg(LOG_SEV_INFO, "inet6 pkts out %lld\n",
						pkt_counters->inet6_out_pkts);
		break;
	case RCMODE_INET6_TO_INET6:
		log_msg(LOG_SEV_INFO, "inet6 pkts in %lld, ",
						pkt_counters->inet6_in_pkts);
		log_msg(LOG_SEV_INFO, "inet6 pkts out %lld\n",
						pkt_counters->inet6_out_pkts);
		break;
	case RCMODE_INET6_TO_INET:
		log_msg(LOG_SEV_INFO, "inet6 pkts in %lld, ",
						pkt_counters->inet6_in_pkts);
		log_msg(LOG_SEV_INFO, "inet pkts out %lld\n",
						pkt_counters->inet_out_pkts);
		break;
	case RCMODE_INET6_TO_INET_INET6:
		log_msg(LOG_SEV_INFO, "inet6 pkts in %lld, ",
						pkt_counters->inet6_in_pkts);
		log_msg(LOG_SEV_INFO, "inet pkts out %lld, ",
						pkt_counters->inet_out_pkts);
		log_msg(LOG_SEV_INFO, "inet6 pkts out %lld\n",
						pkt_counters->inet6_out_pkts);
		break;
	default:
		break;


	}

	log_debug_med("%s() exit\n", __func__);

}


void exit_program(void)
{


	log_debug_med("%s() entry\n", __func__);

	close_sockets(&sock_fds);

	log_packet_counters(prog_parms.rc_mode, &pkt_counters);

	cleanup_prog_parms(&prog_parms);

	log_debug_med("%s() exit\n", __func__);

	log_close();

	exit(EXIT_SUCCESS);

}
