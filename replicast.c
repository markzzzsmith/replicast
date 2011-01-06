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
	VPO_ERR_UNKNOWN_OPT,
	VPO_ERR_NO_SRC_GRP,
	VPO_ERR_MULTI_SRC_GRPS,
	VPO_ERR_NO_DST_GRPS,
	VPO_MODE_INETINETINET6,
	VPO_MODE_INETINET,
	VPO_MODE_INETINET6,
	VPO_MODE_INET6INETINET6,
	VPO_MODE_INET6INET,
	VPO_MODE_INET6INET6,
	VPO_ERR_UNKNOWN,
};

enum VALIDATE_PROG_OPTS_VALS {
	VPOV_ERR_SRC_GRP_ADDR,
	VPOV_ERR_IF_ADDR,
	VPOV_ERR_SRC_PORT,
	VPOV_ERR_DST_PORT,
	VPOV_ERR_INET_DST_GRP,
	VPOV_ERR_INET_TX_TTL_RANGE,
	VPOV_ERR_INET_OUT_INTF,
	VPOV_ERR_INET6_OUT_INTF,
	VPOV_ERR_INET6_DST_GRP,
	VPOV_ERR_INET6_TX_HOPS_RANGE,
	VPOV_ERR_UNKNOWN_ERR,
	VPOV_OPTS_VALS_VALID,
};

enum OPT_ERR {
	OE_UNKNOWN_OPT,
	OE_NO_SRC_GRP,
	OE_MULTI_SRC_GRPS,
	OE_NO_DST_GRPS,
	OE_SRC_GRP_ADDR,
	OE_IF_ADDR,
	OE_SRC_PORT,
	OE_DST_PORT,
	OE_INET_DST_GRPS,
	OE_INET_TX_TTL_RANGE,
	OE_INET_OUT_INTF,
	OE_INET6_OUT_INTF,
	OE_INET6_DST_GRPS,
	OE_INET6_TX_HOPS_RANGE,
	OE_UNKNOWN_ERROR,
};


enum REPLICAST_MODE {
	RCMODE_UNKNOWN,
	RCMODE_HELP,
	RCMODE_ERROR,
	RCMODE_INET_TO_INET,
	RCMODE_INET_TO_INET6,
	RCMODE_INET_TO_INET_INET6,
	RCMODE_INET6_TO_INET6,
	RCMODE_INET6_TO_INET,
	RCMODE_INET6_TO_INET_INET6,
};



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
	unsigned int inet_tx_sock_mc_dests_set;
	char *inet_tx_sock_mc_dests_str;

	unsigned int inet6_rx_sock_mcgroup_set;
	char *inet6_rx_sock_mcgroup_str;

	unsigned int inet6_tx_mc_sock_mc_hops_set;
	char *inet6_tx_mc_sock_mc_hops_str;
	unsigned int inet6_tx_mc_sock_mc_loop_set;
	unsigned int inet6_tx_mc_sock_out_intf_set;
	char *inet6_tx_mc_sock_out_intf_str;
	unsigned int inet6_tx_mc_sock_mc_dests_set;
	char *inet6_tx_mc_sock_mc_dests_str;
};


struct program_parameters {
	enum REPLICAST_MODE rc_mode;
	unsigned int become_daemon;
	struct inet_rx_mc_sock_params inet_rx_sock_parms;
	struct inet_tx_mc_sock_params inet_tx_sock_parms;
	struct inet6_rx_mc_sock_params inet6_rx_sock_parms;
	struct inet6_tx_mc_sock_params inet6_tx_sock_parms;
};


void log_prog_help(void);

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

void log_inet_rx_sock_parms(const struct inet_rx_mc_sock_params
								*inet_rx_parms);

void log_inet6_rx_sock_parms(const struct inet6_rx_mc_sock_params
							*inet6_rx_parms);

void log_inet_tx_sock_parms(const struct inet_tx_mc_sock_params
							*inet_tx_parms);

void log_inet6_tx_sock_parms(const struct inet6_tx_mc_sock_params
							*inet6_tx_parms);

void log_opt_error(enum OPT_ERR option_err,
		   const char *err_str_parm);


void cleanup_prog_parms(struct program_parameters *prog_parms);

void init_sock_fds(struct socket_fds *sock_fds);

void inet_to_inet_mcast(int *inet_in_sock_fd,
			struct inet_rx_mc_sock_params rx_sock_parms,
			int *inet_out_sock_fd,
			struct inet_tx_mc_sock_params tx_sock_parms,
			struct packet_counters *pkt_counters);

void inet_to_inet6_mcast(int *inet_in_sock_fd,
			 struct inet_rx_mc_sock_params rx_sock_parms,
			 int *inet6_out_sock_fd,
			 struct inet6_tx_mc_sock_params tx_sock_parms,
			 struct packet_counters *pkt_counters);

void inet_to_inet_inet6_mcast(int *inet_in_sock_fd,
			      struct inet_rx_mc_sock_params rx_sock_parms,
			      int *inet_out_sock_fd,
			      struct inet_tx_mc_sock_params inet_tx_sock_parms,
			      int *inet6_out_sock_fd,
			      struct inet6_tx_mc_sock_params
							inet6_tx_sock_parms,
			      struct packet_counters *pkt_counters);

void inet6_to_inet6_mcast(int *inet6_in_sock_fd,
			  struct inet6_rx_mc_sock_params rx_sock_parms,
			  int *inet6_out_sock_fd,
			  struct inet6_tx_mc_sock_params tx_sock_parms,
			  struct packet_counters *pkt_counters);

void inet6_to_inet_mcast(int *inet6_in_sock_fd,
			 struct inet6_rx_mc_sock_params rx_sock_parms,
			 int *inet_out_sock_fd,
			 struct inet_tx_mc_sock_params tx_sock_parms,
			 struct packet_counters *pkt_counters);

void inet6_to_inet_inet6_mcast(int *inet6_in_sock_fd,
			       struct inet6_rx_mc_sock_params rx_sock_parms,
			       int *inet_out_sock_fd,
			       struct inet_tx_mc_sock_params inet_tx_sock_parms,
			       int *inet6_out_sock_fd,
			       struct inet6_tx_mc_sock_params
							inet6_tx_sock_parms,
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
 		  const struct sockaddr_in inet_mc_dests[]);

int inet6_tx_mcast(const int sock_fd,
		   const void *pkt,
		   const size_t pkt_len,
 		   const struct sockaddr_in6 inet6_mc_dests[]);

void close_sockets(const struct socket_fds *sock_fds);

void log_packet_counters(const struct packet_counters *pkt_counters);

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
	case RCMODE_INET_TO_INET:
		log_debug_med("%s() rc_mode = RCMODE_INET_TO_INET\n", __func__);
		install_usr_signal_handlers();
		if (prog_parms.become_daemon) {
			daemonise();
		}
		log_prog_banner();
		log_prog_parms(&prog_parms);
		inet_to_inet_mcast(&sock_fds.inet_in_sock_fd,
				   prog_parms.inet_rx_sock_parms,
				   &sock_fds.inet_out_sock_fd,
				   prog_parms.inet_tx_sock_parms,
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
		inet_to_inet6_mcast(&sock_fds.inet_in_sock_fd,
				    prog_parms.inet_rx_sock_parms,
				    &sock_fds.inet6_out_sock_fd,
				    prog_parms.inet6_tx_sock_parms,
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
		inet_to_inet_inet6_mcast(&sock_fds.inet_in_sock_fd,
					 prog_parms.inet_rx_sock_parms,
					 &sock_fds.inet_out_sock_fd,
					 prog_parms.inet_tx_sock_parms,
					 &sock_fds.inet6_out_sock_fd,
					 prog_parms.inet6_tx_sock_parms,
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
		inet6_to_inet6_mcast(&sock_fds.inet6_in_sock_fd,
				     prog_parms.inet6_rx_sock_parms,
				     &sock_fds.inet6_out_sock_fd,
				     prog_parms.inet6_tx_sock_parms,
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
		inet6_to_inet_mcast(&sock_fds.inet6_in_sock_fd,
				    prog_parms.inet6_rx_sock_parms,
				    &sock_fds.inet_out_sock_fd,
				    prog_parms.inet_tx_sock_parms,
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
		inet6_to_inet_inet6_mcast(&sock_fds.inet6_in_sock_fd,
				    prog_parms.inet6_rx_sock_parms,
				    &sock_fds.inet_out_sock_fd,
				    prog_parms.inet_tx_sock_parms,
				    &sock_fds.inet6_out_sock_fd,
				    prog_parms.inet6_tx_sock_parms,
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

	log_msg(LOG_SEV_INFO, "\nreplicate IPv4 or IPv6 UDP datagrams to ");
	log_msg(LOG_SEV_INFO, "IPv4 and/or IPv6 destinations.\n");

	log_msg(LOG_SEV_INFO, "\ncommand line options:\n");

	log_msg(LOG_SEV_INFO, "-help\n");
	log_msg(LOG_SEV_INFO, "-nodaemon\n");

	log_msg(LOG_SEV_INFO, "-4src <addr>[%<ifname>|<ifaddr>]:<port>\n");
	log_msg(LOG_SEV_INFO, "\te.g. -4src 224.0.0.35:1234\n");
	log_msg(LOG_SEV_INFO, "\te.g. -4src 224.0.0.35%%eth0:1234\n");
	log_msg(LOG_SEV_INFO, "\te.g. -4src 224.0.0.35%%192.168.1.1:1234\n");

	log_msg(LOG_SEV_INFO, "-6src <[addr]>[%<ifname>]:<port>\n");
	log_msg(LOG_SEV_INFO, "\te.g. -6src [ff05::35]:1234\n");
	log_msg(LOG_SEV_INFO, "\te.g. -6src [ff05::35%%eth0]:1234\n");

	log_msg(LOG_SEV_INFO, "-4dsts <addr>:<port>,<addr>:port,...\n");
	log_msg(LOG_SEV_INFO, "\te.g. -4dsts 224.0.0.36:1234,");
	log_msg(LOG_SEV_INFO, "224.0.0.37:5678,224.0.0.38:9012\n");

	log_msg(LOG_SEV_INFO, "-4ttl <ttl>\n");
	log_msg(LOG_SEV_INFO, "\te.g. -4ttl 32\n");

	log_msg(LOG_SEV_INFO, "-4loop\n");

	log_msg(LOG_SEV_INFO, "-4outif <ifname>\n");
	log_msg(LOG_SEV_INFO, "\te.g. -4outif eth0\n");

	log_msg(LOG_SEV_INFO, "-6dsts <[addr]>:<port>,<[addr]>:<port>,...\n");
	log_msg(LOG_SEV_INFO, "\te.g. -6dsts [ff05::36]:1234,");
	log_msg(LOG_SEV_INFO, "[ff05::37]:5678,[ff05::39]:9012\n");

	log_msg(LOG_SEV_INFO, "-6hops <hop count>\n");
	log_msg(LOG_SEV_INFO, "\te.g. -6hops 16\n");

	log_msg(LOG_SEV_INFO, "-6loop\n");

	log_msg(LOG_SEV_INFO, "-6outif <ifname>\n");
	log_msg(LOG_SEV_INFO, "\te.g. -6outif eth0\n");

	log_msg(LOG_SEV_INFO, "\nsignals:\n");

	log_msg(LOG_SEV_INFO, "SIGUSR1 - log current UDP datagram rx and tx ");
	log_msg(LOG_SEV_INFO, "stats.\n");
	log_msg(LOG_SEV_INFO, "SIGUSR2 - log program parameters.\n");

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
	case VPO_ERR_UNKNOWN_OPT:
		prog_parms->rc_mode = RCMODE_ERROR;
		log_opt_error(OE_UNKNOWN_OPT, NULL);
		break;
	case VPO_ERR_NO_SRC_GRP:
		prog_parms->rc_mode = RCMODE_ERROR;
		log_opt_error(OE_NO_SRC_GRP, NULL);
		break;
	case VPO_ERR_MULTI_SRC_GRPS:
		prog_parms->rc_mode = RCMODE_ERROR;
		log_opt_error(OE_MULTI_SRC_GRPS, NULL);
		break;
	case VPO_ERR_NO_DST_GRPS:
		prog_parms->rc_mode = RCMODE_ERROR;
		log_opt_error(OE_NO_DST_GRPS, NULL);
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
	prog_opts->inet_tx_sock_mc_dests_set = 0;
	prog_opts->inet_tx_sock_mc_dests_str = NULL;

	prog_opts->inet6_rx_sock_mcgroup_set = 0;
	prog_opts->inet6_rx_sock_mcgroup_str = NULL;

	prog_opts->inet6_tx_mc_sock_mc_hops_set = 0;
	prog_opts->inet6_tx_mc_sock_mc_hops_str = NULL;
	prog_opts->inet6_tx_mc_sock_mc_loop_set = 0;
	prog_opts->inet6_tx_mc_sock_out_intf_set = 0;
	prog_opts->inet6_tx_mc_sock_out_intf_str = NULL;
	prog_opts->inet6_tx_mc_sock_mc_dests_set = 0;
	prog_opts->inet6_tx_mc_sock_mc_dests_str = NULL;
	
	log_debug_med("%s() exit\n", __func__);

}


void init_prog_parms(struct program_parameters *prog_parms)
{


	log_debug_med("%s() entry\n", __func__);

	prog_parms->rc_mode = RCMODE_UNKNOWN;

	prog_parms->become_daemon = 1;

	prog_parms->inet_rx_sock_parms.mc_group.s_addr = ntohl(INADDR_NONE);
	prog_parms->inet_rx_sock_parms.port = 0;
	prog_parms->inet_rx_sock_parms.in_intf_addr.s_addr = ntohl(INADDR_ANY);

	prog_parms->inet_tx_sock_parms.mc_ttl = 1;
	prog_parms->inet_tx_sock_parms.mc_loop = 0;
	prog_parms->inet_tx_sock_parms.out_intf_addr.s_addr = ntohl(INADDR_ANY);
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

	log_debug_med("%s() exit\n", __func__);

}


void get_prog_opts_cmdline(int argc, char *argv[],
			   struct program_options *prog_opts)
{
	enum CMDLINE_OPTS {
		CMDLINE_OPT_HELP = 1,
		CMDLINE_OPT_NODAEMON,
		CMDLINE_OPT_4SRC,
		CMDLINE_OPT_4TTL,
		CMDLINE_OPT_4LOOP,
		CMDLINE_OPT_4OUTIF,
		CMDLINE_OPT_4DSTS,
		CMDLINE_OPT_6SRC,
		CMDLINE_OPT_6HOPS,
		CMDLINE_OPT_6LOOP,
		CMDLINE_OPT_6OUTIF,
		CMDLINE_OPT_6DSTS,
	};
	struct option cmdline_opts[] = {
		{"help", no_argument, NULL, CMDLINE_OPT_HELP},
		{"nodaemon", no_argument, NULL, CMDLINE_OPT_NODAEMON},
		{"4src", required_argument, NULL, CMDLINE_OPT_4SRC},
		{"4ttl", required_argument, NULL, CMDLINE_OPT_4TTL},
		{"4loop", no_argument, NULL, CMDLINE_OPT_4LOOP},
		{"4outif", required_argument, NULL, CMDLINE_OPT_4OUTIF},
		{"4dsts", required_argument, NULL, CMDLINE_OPT_4DSTS},
		{"6src", required_argument, NULL, CMDLINE_OPT_6SRC},
		{"6hops", required_argument, NULL, CMDLINE_OPT_6HOPS},
		{"6loop", no_argument, NULL, CMDLINE_OPT_6LOOP},
		{"6outif", required_argument, NULL, CMDLINE_OPT_6OUTIF},
		{"6dsts", required_argument, NULL, CMDLINE_OPT_6DSTS},
		{0, 0, 0, 0}
	};
	enum CMDLINE_OPTS ret;


	log_debug_med("%s() entry\n", __func__);

	opterr = 0;

	ret = getopt_long_only(argc, argv, ":", cmdline_opts, NULL);
	log_debug_low("%s: getopt_long_only() = %d, %c\n", __func__, ret, ret);
	while ((ret != -1) && (!prog_opts->help_set) &&
						(!prog_opts->unknown_opt_set)) {
		switch (ret) {
		case CMDLINE_OPT_HELP:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_HELP\n", __func__);
			prog_opts->help_set = 1;
			break;
		case CMDLINE_OPT_NODAEMON:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_NODAEMON\n", __func__);
			prog_opts->no_daemon_set = 1;
			break;
		case CMDLINE_OPT_4SRC:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_4SRC\n", __func__);
			prog_opts->inet_rx_sock_mcgroup_set = 1;
			prog_opts->inet_rx_sock_mcgroup_str = optarg;
			break;
		case CMDLINE_OPT_4TTL:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_4TTL\n", __func__);
			prog_opts->inet_tx_sock_mc_ttl_set = 1;
			prog_opts->inet_tx_sock_mc_ttl_str = optarg;
			break;
		case CMDLINE_OPT_4LOOP:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_4LOOP\n", __func__);
			prog_opts->inet_tx_sock_mc_loop_set = 1;
			break;
		case CMDLINE_OPT_4OUTIF:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_4OUTIF\n", __func__);
			prog_opts->inet_tx_sock_out_intf_set = 1;
			prog_opts->inet_tx_sock_out_intf_str = optarg;
			break;
		case CMDLINE_OPT_4DSTS:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_4DSTS\n", __func__);
			prog_opts->inet_tx_sock_mc_dests_set = 1;
			prog_opts->inet_tx_sock_mc_dests_str = optarg;
			break;
		case CMDLINE_OPT_6SRC:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_6SRC\n", __func__);
			prog_opts->inet6_rx_sock_mcgroup_set = 1;
			prog_opts->inet6_rx_sock_mcgroup_str = optarg;
			break;
		case CMDLINE_OPT_6HOPS:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_6HOPS\n", __func__);
			prog_opts->inet6_tx_mc_sock_mc_hops_set = 1;
			prog_opts->inet6_tx_mc_sock_mc_hops_str = optarg;
			break;
		case CMDLINE_OPT_6LOOP:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_6LOOP\n", __func__);
			prog_opts->inet6_tx_mc_sock_mc_loop_set = 1;
			break;
		case CMDLINE_OPT_6OUTIF:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_6OUTIF\n", __func__);
			prog_opts->inet6_tx_mc_sock_out_intf_set = 1;
			prog_opts->inet6_tx_mc_sock_out_intf_str = optarg;
			break;
		case CMDLINE_OPT_6DSTS:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_6DSTS\n", __func__);
			prog_opts->inet6_tx_mc_sock_mc_dests_set = 1;
			prog_opts->inet6_tx_mc_sock_mc_dests_str = optarg;
			break;
		case ':':
			log_debug_low("%s: getopt_long_only() = missing "
				"option parameter\n", __func__);
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
	
	if (prog_opts->unknown_opt_set) {
		log_debug_low("%s() return VPO_ERR_UNKNOWN_OPT\n", __func__);
		log_debug_med("%s() exit\n", __func__);
		return VPO_ERR_UNKNOWN_OPT;
	}

	if (!prog_opts->inet_rx_sock_mcgroup_set &&
				!prog_opts->inet6_rx_sock_mcgroup_set) {
		log_debug_low("%s() return VPO_ERR_NO_SRC_GRP\n", __func__);
		log_debug_med("%s() exit\n", __func__);
		return VPO_ERR_NO_SRC_GRP;
	}

	if (prog_opts->inet_rx_sock_mcgroup_set &&
				prog_opts->inet6_rx_sock_mcgroup_set) {
		log_debug_low("%s() return VPO_ERR_MULTI_SRC_GRPS\n", __func__);
		log_debug_med("%s() exit\n", __func__);
		return VPO_ERR_MULTI_SRC_GRPS;
	}


	if (!prog_opts->inet_tx_sock_mc_dests_set &&
				!prog_opts->inet6_tx_mc_sock_mc_dests_set) {
		log_debug_low("%s() return VPO_ERR_NO_DST_GRPS\n", __func__);
		log_debug_med("%s() exit\n", __func__);
		return VPO_ERR_NO_DST_GRPS;
	}

	if (prog_opts->inet_rx_sock_mcgroup_set) {
		if (prog_opts->inet_tx_sock_mc_dests_set &&
                                prog_opts->inet6_tx_mc_sock_mc_dests_set) {
			log_debug_low("%s() return VPO_MODE_INETINETINET6\n",
				__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPO_MODE_INETINETINET6;
		} else if (prog_opts->inet_tx_sock_mc_dests_set) {
			log_debug_low("%s() return VPO_MODE_INETINET\n",
				__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPO_MODE_INETINET;	
		} else if (prog_opts->inet6_tx_mc_sock_mc_dests_set) {
			log_debug_low("%s() return VPO_MODE_INETINET6\n",
				__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPO_MODE_INETINET6;
		}
	}

	if (prog_opts->inet6_rx_sock_mcgroup_set) {
		if (prog_opts->inet_tx_sock_mc_dests_set &&
                                prog_opts->inet6_tx_mc_sock_mc_dests_set) {
			log_debug_low("%s() return VPO_MODE_INET6INETINET6\n",
				__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPO_MODE_INET6INETINET6;
		} else if (prog_opts->inet_tx_sock_mc_dests_set) {
			log_debug_low("%s() return VPO_MODE_INET6INET\n",
				__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPO_MODE_INET6INET;	
		} else if (prog_opts->inet6_tx_mc_sock_mc_dests_set) {
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
	enum aip_ptoh_errors aip_ptoh_err;
	int tx_ttl;
	int tx_hops;
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
			&prog_parms->inet_rx_sock_parms.mc_group,
			&in_intf_addr,
			&prog_parms->inet_rx_sock_parms.port,
			&aip_ptoh_err);
		if (ret == -1) {
			switch (aip_ptoh_err) {
			case AIP_PTOX_ERR_BAD_ADDR:
				log_debug_low("%s() return "
					"VPOV_ERR_SRC_GRP_ADDR\n",
					__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_SRC_GRP_ADDR;
				break;
			case AIP_PTOX_ERR_BAD_IF_ADDR:
				log_debug_low("%s() return "
					"VPOV_ERR_IF_ADDR\n",
					__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_IF_ADDR;
				break;
			case AIP_PTOX_ERR_BAD_PORT:
				log_debug_low("%s() return "
					"VPOV_ERR_SRC_PORT\n",
					__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_SRC_PORT;
				break;
			default:
				log_debug_low("%s() return "
					"VPOV_ERR_UNKNOWN_ERR\n",
					__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_UNKNOWN_ERR;
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
			&prog_parms->inet6_rx_sock_parms.mc_group,
			&prog_parms->inet6_rx_sock_parms.in_intf_idx,
			&prog_parms->inet6_rx_sock_parms.port,
			&aip_ptoh_err);
		if (ret == -1) {
			switch (aip_ptoh_err) {
			case AIP_PTOX_ERR_BAD_ADDR:
				log_debug_low("%s() return "
					"VPOV_ERR_SRC_GRP_ADDR\n",
					__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_SRC_GRP_ADDR;
				break;
			case AIP_PTOX_ERR_BAD_PORT:
				log_debug_low("%s() return "
					"VPOV_ERR_SRC_PORT\n",
					__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_SRC_PORT;
				break;
			default:
				log_debug_low("%s() return "
					"VPOV_ERR_UNKNOWN_ERR\n",
					__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_UNKNOWN_ERR;
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

	if (prog_opts->inet_tx_sock_mc_dests_set) {
		log_debug_low("%s() prog_opts->inet_tx_sock_mc_dests_set\n",
								__func__);
		ret = ap_pton_inet_csv(
				prog_opts->inet_tx_sock_mc_dests_str,
				&prog_parms->inet_tx_sock_parms.mc_dests,
				0,
				1,
				err_str_parm,
				err_str_size);
		if (ret == -1) {
			log_debug_low("%s() return VPOV_ERR_INET_DST_GRP\n",
					__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPOV_ERR_INET_DST_GRP;
		} else {
			prog_parms->inet_tx_sock_parms.mc_dests_num = ret;
		}

		if (prog_opts->inet_tx_sock_mc_ttl_set) {
			log_debug_low("%s() prog_opts->", __func__);
			log_debug_low("inet_tx_sock_mc_ttl_set\n");
			tx_ttl = atoi(prog_opts->inet_tx_sock_mc_ttl_str);
			if ((tx_ttl < 0) || (tx_ttl > 255)) {
				log_debug_low("%s() return "
					"VPOV_ERR_INET_TX_TTL_RANGE\n",
						__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_INET_TX_TTL_RANGE;
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

	if (prog_opts->inet6_tx_mc_sock_mc_dests_set) {
		log_debug_low("%s() prog_opts->inet6_tx_sock_mc_dests_set\n",
								__func__);

		ret = ap_pton_inet6_csv(
				prog_opts->inet6_tx_mc_sock_mc_dests_str,
				&prog_parms->inet6_tx_sock_parms.mc_dests,
				0,
				1,
				err_str_parm,
				err_str_size);
		if (ret == -1) {
			log_debug_low("%s() return VPOV_ERR_INET6_DST_GRP\n",
					__func__);
			log_debug_med("%s() exit\n", __func__);
			return VPOV_ERR_INET6_DST_GRP;
		} else {
			prog_parms->inet6_tx_sock_parms.mc_dests_num = ret;
		}


		if (prog_opts->inet6_tx_mc_sock_mc_hops_set) {
			tx_hops = atoi(prog_opts->inet6_tx_mc_sock_mc_hops_str);
			if ((tx_hops < 0) || (tx_hops > 255)) {
				log_debug_low("%s() return "
					"VPOV_ERR_INET6_TX_HOPS_RANGE\n",
						__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_INET6_TX_HOPS_RANGE;
			} else {
				prog_parms->inet6_tx_sock_parms.mc_hops =
								tx_hops;
			}
		}

		if (prog_opts->inet6_tx_mc_sock_mc_loop_set) {
			log_debug_low("%s() prog_opts->", __func__);
			log_debug_low("inet6_tx_mc_sock_mc_loop_set\n");
			prog_parms->inet6_tx_sock_parms.mc_loop = 1;
		}

		if (prog_opts->inet6_tx_mc_sock_out_intf_set) {
			out_intf_idx = if_nametoindex(prog_opts->
						inet6_tx_mc_sock_out_intf_str);
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
	case VPOV_ERR_SRC_GRP_ADDR:
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
	case VPOV_ERR_INET_DST_GRP:
		log_opt_error(OE_INET_DST_GRPS, NULL);
		break;
	case VPOV_ERR_INET_TX_TTL_RANGE:
		log_opt_error(OE_INET_TX_TTL_RANGE, NULL);
		break;
	case VPOV_ERR_INET_OUT_INTF:
		log_opt_error(OE_INET_OUT_INTF, NULL);
		break;
	case VPOV_ERR_INET6_OUT_INTF:
		log_opt_error(OE_INET6_OUT_INTF, NULL);
		break;
	case VPOV_ERR_INET6_DST_GRP:
		log_opt_error(OE_INET6_DST_GRPS, NULL);
		break;
	case VPOV_ERR_INET6_TX_HOPS_RANGE:
		log_opt_error(OE_INET6_TX_HOPS_RANGE, NULL);
		break;
	case VPOV_ERR_UNKNOWN_ERR:
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


void log_inet_rx_sock_parms(const struct inet_rx_mc_sock_params *inet_rx_parms)
{
	char aip_str[AIP_STR_INET_MAX_LEN + 1];
	const unsigned int aip_str_size = AIP_STR_INET_MAX_LEN + 1;


	log_debug_med("%s() entry\n", __func__);

	log_msg(LOG_SEV_INFO, "inet rx src: ");

	aip_htop_inet(&inet_rx_parms->mc_group, &inet_rx_parms->in_intf_addr,
		inet_rx_parms->port, aip_str, aip_str_size);

	log_msg(LOG_SEV_INFO, "%s\n", aip_str);

	log_debug_med("%s() exit\n", __func__);

}


void log_inet6_rx_sock_parms(const struct inet6_rx_mc_sock_params
							*inet6_rx_parms)
{
	char aip_str[AIP_STR_INET6_MAX_LEN + 1];
	const unsigned int aip_str_size = AIP_STR_INET6_MAX_LEN + 1;


	log_debug_med("%s() entry\n", __func__);

	log_msg(LOG_SEV_INFO, "inet6 rx src: ");

	aip_htop_inet6(&inet6_rx_parms->mc_group, inet6_rx_parms->in_intf_idx,
		inet6_rx_parms->port, aip_str, aip_str_size);

	log_msg(LOG_SEV_INFO, "%s\n", aip_str);

	log_debug_med("%s() exit\n", __func__);

}


void log_inet_tx_sock_parms(const struct inet_tx_mc_sock_params *inet_tx_parms)
{
	const unsigned int ap_str_size = INET_ADDRSTRLEN + 1 + 5 + 1;
	char ap_str[ap_str_size];
	unsigned int mc_dest_num = 0;
	char out_intf_addr_str[INET_ADDRSTRLEN];


	log_debug_med("%s() entry\n", __func__);

	log_msg(LOG_SEV_INFO, "inet tx dsts: ");

	for (mc_dest_num = 0; mc_dest_num < (inet_tx_parms->mc_dests_num - 1);
								mc_dest_num++) {

		ap_htop_inet(&inet_tx_parms->mc_dests[mc_dest_num].sin_addr,
			ntohs(inet_tx_parms->mc_dests[mc_dest_num].sin_port),
			ap_str, ap_str_size);
		log_msg(LOG_SEV_INFO, "%s,", ap_str);
	}

	ap_htop_inet(&inet_tx_parms->mc_dests[mc_dest_num].sin_addr,
		ntohs(inet_tx_parms->mc_dests[mc_dest_num].sin_port),
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

	log_msg(LOG_SEV_INFO, ", ttl %d\n", inet_tx_parms->mc_ttl);

	log_debug_med("%s() exit\n", __func__);

}


void log_inet6_tx_sock_parms(const struct inet6_tx_mc_sock_params
								*inet6_tx_parms)
{
	const unsigned int ap_str_size = 1 + INET6_ADDRSTRLEN + 1 + 1 + 5 + 1;
	char ap_str[ap_str_size];
	unsigned int mc_dest_num = 0;
	char out_intf_name[IFNAMSIZ];


	log_debug_med("%s() entry\n", __func__);

	log_msg(LOG_SEV_INFO, "inet6 tx dsts: ");

	for (mc_dest_num = 0; mc_dest_num < (inet6_tx_parms->mc_dests_num - 1);
								mc_dest_num++) {

		ap_htop_inet6(&inet6_tx_parms->mc_dests[mc_dest_num].sin6_addr,
			ntohs(inet6_tx_parms->mc_dests[mc_dest_num].sin6_port),
			ap_str, ap_str_size);
		log_msg(LOG_SEV_INFO, "%s,", ap_str);
	}

	ap_htop_inet6(&inet6_tx_parms->mc_dests[mc_dest_num].sin6_addr,
		ntohs(inet6_tx_parms->mc_dests[mc_dest_num].sin6_port),
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

	log_msg(LOG_SEV_INFO, ", hops %d\n", inet6_tx_parms->mc_hops);

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
	case OE_NO_SRC_GRP:
		log_msg(LOG_SEV_ERR, "No multicast source group specified.\n");
		break;
	case OE_MULTI_SRC_GRPS:
		log_msg(LOG_SEV_ERR, "Too many multicast source groups.\n");
		break;
	case OE_NO_DST_GRPS:
		log_msg(LOG_SEV_ERR, "No multicast destination group(s) ");
		log_msg(LOG_SEV_ERR, "specified.\n");
		break;
	case OE_SRC_GRP_ADDR:
		log_msg(LOG_SEV_ERR, "Invalid multicast source group.\n");
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
	case OE_INET_DST_GRPS:
		log_msg(LOG_SEV_ERR, "Invalid IPv4 destination group.\n");
		break;
	case OE_INET_TX_TTL_RANGE:
		log_msg(LOG_SEV_ERR, "Invalid IPv4 transmit time-to-live.\n");
		break;
	case OE_INET_OUT_INTF:
		log_msg(LOG_SEV_ERR, "Invalid IPv4 output interface.\n");
		break;
	case OE_INET6_OUT_INTF:
		log_msg(LOG_SEV_ERR, "Invalid IPv6 output interface.\n");
		break;
	case OE_INET6_DST_GRPS:
		log_msg(LOG_SEV_ERR, "Invalid IPv6 destination group.\n");
		break;
	case OE_INET6_TX_HOPS_RANGE:
		log_msg(LOG_SEV_ERR, "Invalid IPv6 transmit hop-count.\n");
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

	if (prog_parms->inet_tx_sock_parms.mc_dests != NULL) {
		free(prog_parms->inet_tx_sock_parms.mc_dests);
		prog_parms->inet_tx_sock_parms.mc_dests = NULL;
	}

	if (prog_parms->inet6_tx_sock_parms.mc_dests != NULL) {
		free(prog_parms->inet6_tx_sock_parms.mc_dests);
		prog_parms->inet6_tx_sock_parms.mc_dests = NULL;
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


void inet_to_inet_mcast(int *inet_in_sock_fd,
			struct inet_rx_mc_sock_params rx_sock_parms,
			int *inet_out_sock_fd,
			struct inet_tx_mc_sock_params tx_sock_parms,
			struct packet_counters *pkt_counters)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;


	log_debug_med("%s() entry\n", __func__);

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
		log_debug_low("%s(): errno == %d, %s\n", __func__, errno,
							strerror(errno));
		if (rx_pkt_len > 0) {
			pkt_counters->inet_in_pkts++;
			inet_tx_mcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms.mc_dests);
			pkt_counters->inet_out_pkts +=
						tx_sock_parms.mc_dests_num;
		}
	}

	log_debug_med("%s() exit\n", __func__);

}


void inet_to_inet6_mcast(int *inet_in_sock_fd,
			 struct inet_rx_mc_sock_params rx_sock_parms,
			 int *inet6_out_sock_fd,
			 struct inet6_tx_mc_sock_params tx_sock_parms,
			 struct packet_counters *pkt_counters)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;


	log_debug_med("%s() entry\n", __func__);

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
		log_debug_low("%s(): errno == %d, %s\n", __func__, errno,
							strerror(errno));
		if (rx_pkt_len > 0) {
			pkt_counters->inet_in_pkts++;
			inet6_tx_mcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms.mc_dests);
			pkt_counters->inet6_out_pkts +=
						tx_sock_parms.mc_dests_num;
		}
	}

	log_debug_med("%s() exit\n", __func__);

}


void inet_to_inet_inet6_mcast(int *inet_in_sock_fd,
			      struct inet_rx_mc_sock_params rx_sock_parms,
			      int *inet_out_sock_fd,
			      struct inet_tx_mc_sock_params inet_tx_sock_parms,
			      int *inet6_out_sock_fd,
			      struct inet6_tx_mc_sock_params
							inet6_tx_sock_parms,
			      struct packet_counters *pkt_counters)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;


	log_debug_med("%s() entry\n", __func__);

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
		log_debug_low("%s(): recv() == %d\n", __func__, rx_pkt_len);
		log_debug_low("%s(): errno == %d, %s\n", __func__, errno,
							strerror(errno));
		if (rx_pkt_len > 0) {
			pkt_counters->inet_in_pkts++;
			inet_tx_mcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				inet_tx_sock_parms.mc_dests);
			pkt_counters->inet_out_pkts +=
					inet_tx_sock_parms.mc_dests_num;
			inet6_tx_mcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				inet6_tx_sock_parms.mc_dests);
			pkt_counters->inet6_out_pkts +=
					inet6_tx_sock_parms.mc_dests_num;
		}
	}

	log_debug_med("%s() exit\n", __func__);

}


void inet6_to_inet6_mcast(int *inet6_in_sock_fd,
			  struct inet6_rx_mc_sock_params rx_sock_parms,
			  int *inet6_out_sock_fd,
			  struct inet6_tx_mc_sock_params tx_sock_parms,
			  struct packet_counters *pkt_counters)
{
	uint8_t pkt_buf[PKT_BUF_SIZE];
	ssize_t rx_pkt_len;


	log_debug_med("%s() entry\n", __func__);

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
		log_debug_low("%s(): errno == %d, %s\n", __func__, errno,
							strerror(errno));
		if (rx_pkt_len > 0) {
			pkt_counters->inet6_in_pkts++;
			inet6_tx_mcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms.mc_dests);
			pkt_counters->inet6_out_pkts +=
						tx_sock_parms.mc_dests_num;
		}
	}

	log_debug_med("\t%s() exit\n", __func__);

}


void inet6_to_inet_mcast(int *inet6_in_sock_fd,
			 struct inet6_rx_mc_sock_params rx_sock_parms,
			 int *inet_out_sock_fd,
			 struct inet_tx_mc_sock_params tx_sock_parms,
			 struct packet_counters *pkt_counters)
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
		log_debug_low("%s(): errno == %d, %s\n", __func__, errno,
							strerror(errno));
		if (rx_pkt_len > 0) {
			pkt_counters->inet6_in_pkts++;
			inet_tx_mcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms.mc_dests);
			pkt_counters->inet_out_pkts +=
						tx_sock_parms.mc_dests_num;
		}
	}

}

void inet6_to_inet_inet6_mcast(int *inet6_in_sock_fd,
			       struct inet6_rx_mc_sock_params rx_sock_parms,
			       int *inet_out_sock_fd,
			       struct inet_tx_mc_sock_params inet_tx_sock_parms,
			       int *inet6_out_sock_fd,
			       struct inet6_tx_mc_sock_params
							inet6_tx_sock_parms,
			       struct packet_counters *pkt_counters)
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
		log_debug_low("%s(): recv() == %d\n", __func__, rx_pkt_len);
		log_debug_low("%s(): errno == %d, %s\n", __func__, errno,
							strerror(errno));
		if (rx_pkt_len > 0) {
			pkt_counters->inet6_in_pkts++;
			inet_tx_mcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				inet_tx_sock_parms.mc_dests);
			pkt_counters->inet_out_pkts +=
						inet_tx_sock_parms.mc_dests_num;
			inet6_tx_mcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				inet6_tx_sock_parms.mc_dests);
			pkt_counters->inet6_out_pkts +=
					inet6_tx_sock_parms.mc_dests_num;
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

        sigint_action->sa_flags = 0;

        sigaction(SIGINT, sigint_action, NULL);

	log_debug_med("%s() exit\n", __func__);

}


void install_sigusr1_handler(struct sigaction *sigusr1_action)
{


	log_debug_med("%s() entry\n", __func__);

	sigusr1_action->sa_handler = sigusr1_handler;
        sigemptyset(&sigusr1_action->sa_mask);
        sigusr1_action->sa_flags = SA_RESTART;

        sigaction(SIGUSR1, sigusr1_action, NULL);

	log_debug_med("%s() exit\n", __func__);

}


void install_sigusr2_handler(struct sigaction *sigusr2_action)
{


	log_debug_med("%s() entry\n", __func__);

	sigusr2_action->sa_handler = sigusr2_handler;
        sigemptyset(&(sigusr2_action->sa_mask));
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

	log_packet_counters(&pkt_counters);

	log_debug_med("%s() exit\n", __func__);


}


void sigusr2_handler(int signum)
{


	log_debug_med("%s() entry\n", __func__);

	log_prog_banner();

	log_prog_parms(&prog_parms);

	log_debug_med("%s() exit\n", __func__);


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


	log_debug_med("%s() entry\n", __func__);

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

	if (IN_MULTICAST(ntohl(mc_group.s_addr))) {
		/* setsockopt() to join mcast group */
		ip_mcast_req.imr_multiaddr = mc_group;
		ip_mcast_req.imr_interface = in_intf_addr;
		ret = setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			&ip_mcast_req, sizeof(ip_mcast_req));
		if (ret == -1) {
			return -1;
		}
	}

	log_debug_med("%s() exit\n", __func__);

	return sock_fd;

}


void close_inet_rx_mc_sock(const int sock_fd)
{


	log_debug_med("%s() entry\n", __func__);

	if (sock_fd != -1) {
		close(sock_fd);
	}

	log_debug_med("%s() exit\n", __func__);

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


	log_debug_med("%s() entry\n", __func__);

	/* socket() */
	sock_fd = socket(AF_INET6, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		log_debug_low("%s(): socket() == %d\n", __func__, sock_fd);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		log_debug_med("%s() exit\n", __func__);
		return -1;
	}

	/* allow duplicate binds to group and UDP port */
	ret = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &one,
		sizeof(one));
	if (ret == -1) {
		log_debug_low("%s(): setsockopt(SO_REUSEADDR) == %d\n",
			__func__, ret);
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		log_debug_med("%s() exit\n", __func__);
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
		log_debug_med("%s() exit\n", __func__);
		return -1;
	}

	if (IN6_IS_ADDR_MULTICAST(&mc_group)) {
		/* setsockopt() to join mcast group */
		ipv6_mcast_req.ipv6mr_multiaddr = mc_group;
		ipv6_mcast_req.ipv6mr_interface = in_intf_idx;
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


void close_inet6_rx_mc_sock(const int sock_fd)
{


	log_debug_med("%s() entry\n", __func__);

	if (sock_fd != -1) {
		close(sock_fd);
	}

	log_debug_med("%s() exit\n", __func__);

}


int open_inet_tx_mc_sock(const unsigned int mc_ttl,
			 const unsigned int mc_loop,
			 const struct in_addr out_intf_addr)
{
	int sock_fd;
	uint8_t ttl;
	uint8_t mcloop;
	int ret;


	log_debug_med("%s() entry\n", __func__);

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

	log_debug_med("%s() exit\n", __func__);

	return sock_fd;

}


void close_inet_tx_mc_sock(int sock_fd)
{


	log_debug_med("%s() entry\n", __func__);

	if (sock_fd != -1) {
		close(sock_fd);
	}

	log_debug_med("%s() exit\n", __func__);

}


int open_inet6_tx_mc_sock(const int mc_hops,
			  const unsigned int mc_loop,
			  const unsigned int out_intf_idx)
{
	int sock_fd;
	int ret;


	log_debug_med("%s() entry\n", __func__);

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

	log_debug_med("%s() exit\n", __func__);

	return sock_fd;
			
}


void close_inet6_tx_mc_sock(int sock_fd)
{


	log_debug_med("%s() entry\n", __func__);

	if (sock_fd != -1) {
		close(sock_fd);
	}

	log_debug_med("%s() exit\n", __func__);

}


int inet_tx_mcast(const int sock_fd,
		  const void *pkt,
		  const size_t pkt_len,
 		  const struct sockaddr_in inet_mc_dests[])
{
	const struct sockaddr_in *sa_mc_dest;
	unsigned int tx_success = 0;
	ssize_t ret;


	log_debug_med("%s() entry\n", __func__);

	sa_mc_dest = inet_mc_dests;

	while (sa_mc_dest->sin_family == AF_INET) {
		ret = sendto(sock_fd, pkt, pkt_len, 0,
			(struct sockaddr *)sa_mc_dest,
			sizeof(struct sockaddr_in));
		if (ret != -1) {
			tx_success++;
		}
		log_debug_low("%s(): sendto() == %d\n", __func__, ret);
		log_debug_low("%s(): errno == %d\n", __func__, errno);

		sa_mc_dest++;
	}

	log_debug_med("%s() exit\n", __func__);

	return tx_success;

}


int inet6_tx_mcast(const int sock_fd,
		   const void *pkt,
		   const size_t pkt_len,
 		   const struct sockaddr_in6 inet6_mc_dests[])
{
	const struct sockaddr_in6 *sa6_mc_dest;
	unsigned int tx_success = 0;
	ssize_t ret;


	log_debug_med("%s() entry\n", __func__);

	sa6_mc_dest = inet6_mc_dests;

	while (sa6_mc_dest->sin6_family == AF_INET6) {
		ret = sendto(sock_fd, pkt, pkt_len, 0, 	
			(struct sockaddr *)sa6_mc_dest,
			sizeof(struct sockaddr_in6));
		if (ret != -1) {
			tx_success++;
		}
		log_debug_low("%s(): sendto() == %d\n", __func__, ret);
		log_debug_low("%s(): errno == %d\n", __func__, errno);

		sa6_mc_dest++;
	}
	
	log_debug_med("%s() exit\n", __func__);

	return tx_success;

}


void close_sockets(const struct socket_fds *sock_fds)
{


	log_debug_med("%s() entry\n", __func__);

	close_inet_rx_mc_sock(sock_fds->inet_in_sock_fd);
	close_inet6_rx_mc_sock(sock_fds->inet6_in_sock_fd);
	close_inet_tx_mc_sock(sock_fds->inet_out_sock_fd);
	close_inet6_tx_mc_sock(sock_fds->inet6_out_sock_fd);

	log_debug_med("%s() exit\n", __func__);

}


void log_packet_counters(const struct packet_counters *pkt_counters)
{


	log_msg(LOG_SEV_INFO, "inet pkts in %lld, ",
						pkt_counters->inet_in_pkts);
	log_msg(LOG_SEV_INFO, "inet pkts out %lld, ",
						pkt_counters->inet_out_pkts);

	log_msg(LOG_SEV_INFO, "inet6 pkts in %lld, ",
						pkt_counters->inet6_in_pkts);
	log_msg(LOG_SEV_INFO, "inet6 pkts out %lld\n",
						pkt_counters->inet6_out_pkts);

}


void exit_program(void)
{


	log_debug_med("%s() entry\n", __func__);

	close_sockets(&sock_fds);

	log_packet_counters(&pkt_counters);

	cleanup_prog_parms(&prog_parms);

	log_debug_med("%s() exit\n", __func__);

	log_close();

	exit(EXIT_SUCCESS);

}
