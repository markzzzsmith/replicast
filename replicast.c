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
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "global.h"
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
	VPOV_ERR_OUT_INTF,
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
	OE_INET_DST_GRP,
	OE_INET_TX_TTL_RANGE,
	OE_OUT_INTF,
	OE_INET6_TX_HOPS_RANGE,
	OE_UNKNOWN_ERROR,
};


enum REPLICAST_MODE {
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


struct program_options {
	unsigned int help_set;
	unsigned int unknown_opt_set;
	char *unknown_opt_str;

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
	struct inet_rx_mc_sock_params inet_rx_sock_parms;
	struct inet_tx_mc_sock_params inet_tx_sock_parms;
	struct inet6_rx_mc_sock_params inet6_rx_sock_parms;
	struct inet6_tx_mc_sock_params inet6_tx_sock_parms;
};


enum REPLICAST_MODE get_prog_parms(int argc, char *argv[],
				   struct program_options *prog_opts,
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


void log_opt_error(enum OPT_ERR option_err,
		   const char *err_str_parm);


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

struct program_parameters prog_parms;


int main(int argc, char *argv[])
{
	struct program_options prog_opts;
	char *err_str = NULL;
	enum REPLICAST_MODE rc_mode;


	atexit(close_sockets);

	log_set_detail_level(LOG_SEV_DEBUG_LOW);


	rc_mode = get_prog_parms(argc, argv, &prog_opts, &prog_parms,
								err_str, 0);
	switch (rc_mode) {
	case RCMODE_INET_TO_INET:
		inet_to_inet_mcast(&inet_in_sock_fd,
				   prog_parms.inet_rx_sock_parms,
				   &inet_out_sock_fd,
				   prog_parms.inet_tx_sock_parms);
		break;
	case RCMODE_INET_TO_INET6:
		inet_to_inet6_mcast(&inet_in_sock_fd,
				    prog_parms.inet_rx_sock_parms,
				    &inet6_out_sock_fd,
				    prog_parms.inet6_tx_sock_parms);
		break;
	case RCMODE_INET_TO_INET_INET6:
		inet_to_inet_inet6_mcast(&inet_in_sock_fd,
					 prog_parms.inet_rx_sock_parms,
					 &inet_out_sock_fd,
					 prog_parms.inet_tx_sock_parms,
					 &inet6_out_sock_fd,
					 prog_parms.inet6_tx_sock_parms);
		break;
	case RCMODE_INET6_TO_INET6:
		inet6_to_inet6_mcast(&inet6_in_sock_fd,
				     prog_parms.inet6_rx_sock_parms,
				     &inet6_out_sock_fd,
				     prog_parms.inet6_tx_sock_parms);
		break;
	case RCMODE_INET6_TO_INET:
		inet6_to_inet_mcast(&inet6_in_sock_fd,
				    prog_parms.inet6_rx_sock_parms,
				    &inet_out_sock_fd,
				    prog_parms.inet_tx_sock_parms);
		break;
	case RCMODE_INET6_TO_INET_INET6:
		inet6_to_inet_inet6_mcast(&inet6_in_sock_fd,
				    prog_parms.inet6_rx_sock_parms,
				    &inet_out_sock_fd,
				    prog_parms.inet_tx_sock_parms,
				    &inet6_out_sock_fd,
				    prog_parms.inet6_tx_sock_parms);
		break;
	case RCMODE_ERROR:
		exit(EXIT_FAILURE);
		break;
	}

}


enum REPLICAST_MODE get_prog_parms(int argc, char *argv[],
				   struct program_options *prog_opts,
				   struct program_parameters *prog_parms,
				   char err_str[],
				   const unsigned int err_str_size)
{
	enum VALIDATE_PROG_OPTS vpo;
	enum REPLICAST_MODE rcmode = RCMODE_ERROR;
	int vpo_values_ret = 0;


	log_debug_med("%s() entry\n", __func__);

	init_prog_opts(prog_opts);
	init_prog_parms(prog_parms);

	get_prog_opts_cmdline(argc, argv, prog_opts);

	vpo = validate_prog_opts(prog_opts);

	switch (vpo) {
	case VPO_HELP:
		rcmode = RCMODE_HELP;
		break;
	case VPO_ERR_UNKNOWN_OPT:
		rcmode = RCMODE_ERROR;
		log_opt_error(OE_UNKNOWN_OPT, NULL);
		break;
	case VPO_ERR_NO_SRC_GRP:
		rcmode = RCMODE_ERROR;
		log_opt_error(OE_NO_SRC_GRP, NULL);
		break;
	case VPO_ERR_MULTI_SRC_GRPS:
		rcmode = RCMODE_ERROR;
		log_opt_error(OE_MULTI_SRC_GRPS, NULL);
		break;
	case VPO_ERR_NO_DST_GRPS:
		rcmode = RCMODE_ERROR;
		log_opt_error(OE_NO_DST_GRPS, NULL);
		break;
	case VPO_MODE_INETINETINET6:
		rcmode = RCMODE_INET6_TO_INET_INET6;
		break;
	case VPO_MODE_INETINET:
		rcmode = RCMODE_INET_TO_INET;
		vpo_values_ret = validate_prog_opts_values(prog_opts,
							   prog_parms,
							   err_str,
							   err_str_size);		
		if (vpo_values_ret == -1) {
			rcmode = RCMODE_ERROR;
		}
		break;
	case VPO_MODE_INETINET6:
		rcmode = RCMODE_INET_TO_INET6;
		vpo_values_ret = validate_prog_opts_values(prog_opts,
							   prog_parms,
							   err_str,
							   err_str_size);		
		if (vpo_values_ret == -1) {
			rcmode = RCMODE_ERROR;
		}
		break;
	case VPO_MODE_INET6INETINET6:
		rcmode = RCMODE_INET6_TO_INET_INET6;
		vpo_values_ret = validate_prog_opts_values(prog_opts,
							   prog_parms,
							   err_str,
							   err_str_size);		
		if (vpo_values_ret == -1) {
			rcmode = RCMODE_ERROR;
		}
		break;
	case VPO_MODE_INET6INET:
		rcmode = RCMODE_INET6_TO_INET;
		vpo_values_ret = validate_prog_opts_values(prog_opts,
							   prog_parms,
							   err_str,
							   err_str_size);		
		if (vpo_values_ret == -1) {
			rcmode = RCMODE_ERROR;
		}
		break;
	case VPO_MODE_INET6INET6:
		rcmode = RCMODE_INET6_TO_INET6;
		vpo_values_ret = validate_prog_opts_values(prog_opts,
							   prog_parms,
							   err_str,
							   err_str_size);		
		if (vpo_values_ret == -1) {
			rcmode = RCMODE_ERROR;
		}
		break;
	case VPO_ERR_UNKNOWN:
		rcmode = RCMODE_ERROR;
		log_opt_error(OE_UNKNOWN_ERROR, NULL);
		break;
	default:
		rcmode = RCMODE_HELP;
		break;
	
	}

	log_debug_med("%s() exit\n", __func__);

	return rcmode;

}


void init_prog_opts(struct program_options *prog_opts)
{


	log_debug_med("%s() entry\n", __func__);

	prog_opts->help_set = 0;

	prog_opts->unknown_opt_set = 0;
	prog_opts->unknown_opt_str = NULL;

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

	prog_parms->inet_rx_sock_parms.mc_group.s_addr = INADDR_NONE;
	prog_parms->inet_rx_sock_parms.port = 0;
	prog_parms->inet_rx_sock_parms.in_intf_addr.s_addr = INADDR_ANY;

	prog_parms->inet_tx_sock_parms.mc_ttl = 1;
	prog_parms->inet_tx_sock_parms.mc_loop = 0;
	prog_parms->inet_tx_sock_parms.out_intf_addr.s_addr = INADDR_ANY;
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
		CMDLINE_OPT_4SRCGRP,
		CMDLINE_OPT_4TTL,
		CMDLINE_OPT_4LOOP,
		CMDLINE_OPT_4OUTIF,
		CMDLINE_OPT_4DSTGRPS,
		CMDLINE_OPT_6SRCGRP,
		CMDLINE_OPT_6HOPS,
		CMDLINE_OPT_6LOOP,
		CMDLINE_OPT_6OUTIF,
		CMDLINE_OPT_6DSTGRPS,
	};
	struct option cmdline_opts[] = {
		{"help", no_argument, NULL, CMDLINE_OPT_HELP},
		{"4srcgrp", required_argument, NULL, CMDLINE_OPT_4SRCGRP},
		{"4ttl", required_argument, NULL, CMDLINE_OPT_4TTL},
		{"4loop", no_argument, NULL, CMDLINE_OPT_4LOOP},
		{"4outif", required_argument, NULL, CMDLINE_OPT_4OUTIF},
		{"4dstgrps", required_argument, NULL, CMDLINE_OPT_4DSTGRPS},
		{"6srcgrp", required_argument, NULL, CMDLINE_OPT_6SRCGRP},
		{"6hops", required_argument, NULL, CMDLINE_OPT_6HOPS},
		{"6loop", no_argument, NULL, CMDLINE_OPT_6LOOP},
		{"6outif", required_argument, NULL, CMDLINE_OPT_6OUTIF},
		{"6dstgrps", required_argument, NULL, CMDLINE_OPT_6DSTGRPS},
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
		case CMDLINE_OPT_4SRCGRP:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_4SRCGRP\n", __func__);
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
		case CMDLINE_OPT_4DSTGRPS:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_4DSTGRPS\n", __func__);
			prog_opts->inet_tx_sock_mc_dests_set = 1;
			prog_opts->inet_tx_sock_mc_dests_str = optarg;
			break;
		case CMDLINE_OPT_6SRCGRP:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_6SRCGRP\n", __func__);
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
		case CMDLINE_OPT_6DSTGRPS:
			log_debug_low("%s: getopt_long_only() = "
				"CMDLINE_OPT_6DSTGRPS\n", __func__);
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
			if (in_intf_addr.s_addr != INADDR_NONE) {
				prog_parms->inet_rx_sock_parms.in_intf_addr =
					in_intf_addr;
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
		}
	}

	if (prog_opts->inet_tx_sock_mc_dests_set) {
		log_debug_low("%s() prog_opts->inet_tx_sock_mc_dests_set\n",
								__func__);
		ret = ap_pton_inet_csv(
				prog_opts->inet_tx_sock_mc_dests_str,
				&prog_parms->inet_tx_sock_parms.mc_dests,
				0,
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
					"VPOV_ERR_OUT_INTF\n",
						__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_OUT_INTF;
			}
		}
	}

	if (prog_opts->inet6_tx_mc_sock_mc_dests_set) {
		log_debug_low("%s() prog_opts->inet6_tx_sock_mc_dests_set\n",
								__func__);
		if (prog_opts->inet6_tx_mc_sock_mc_hops_set) {
			tx_hops = atoi(prog_opts->inet6_tx_mc_sock_mc_hops_str);
			if ((tx_hops < 0) || (tx_ttl)) {
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
					"VPOV_ERR_OUT_INTF\n",
						__func__);
				log_debug_med("%s() exit\n", __func__);
				return VPOV_ERR_OUT_INTF;
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
		log_opt_error(OE_INET_DST_GRP, NULL);
		break;
	case VPOV_ERR_INET_TX_TTL_RANGE:
		log_opt_error(OE_INET_TX_TTL_RANGE, NULL);
		break;
	case VPOV_ERR_OUT_INTF:
		log_opt_error(OE_OUT_INTF, NULL);
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


void log_opt_error(enum OPT_ERR option_err,
		   const char *err_str_parm)
{


	log_debug_med("%s() entry\n", __func__);

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
		log_msg(LOG_SEV_ERR, "Invalid source port.\n");
		break;
	case OE_DST_PORT:
		log_msg(LOG_SEV_ERR, "Invalid destination port.\n");
		break;
	case OE_INET_DST_GRP:
		log_msg(LOG_SEV_ERR, "Invalid IPv4 destination group.\n");
		break;
	case OE_INET_TX_TTL_RANGE:
		log_msg(LOG_SEV_ERR, "Invalid IPv4 transmit time-to-live.\n");
		break;
	case OE_OUT_INTF:
		log_msg(LOG_SEV_ERR, "Invalid output interface.\n");
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
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		if (rx_pkt_len > 0) {
			inet_tx_mcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms.mc_dests,
				tx_sock_parms.mc_dests_num);
		}
	}

	log_debug_med("%s() exit\n", __func__);

}


void inet_to_inet6_mcast(int *inet_in_sock_fd,
			struct inet_rx_mc_sock_params rx_sock_parms,
			int *inet6_out_sock_fd,
			struct inet6_tx_mc_sock_params tx_sock_parms)
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
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		if (rx_pkt_len > 0) {
			inet6_tx_mcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms.mc_dests,
				tx_sock_parms.mc_dests_num);
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
							inet6_tx_sock_parms)
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
		if (rx_pkt_len > 0) {
			inet_tx_mcast(*inet_out_sock_fd, pkt_buf, rx_pkt_len,
				inet_tx_sock_parms.mc_dests,
				inet_tx_sock_parms.mc_dests_num);
			inet6_tx_mcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				inet6_tx_sock_parms.mc_dests,
				inet6_tx_sock_parms.mc_dests_num);
		}
	}

	log_debug_med("%s() exit\n", __func__);

}


void inet6_to_inet6_mcast(int *inet6_in_sock_fd,
			  struct inet6_rx_mc_sock_params rx_sock_parms,
			  int *inet6_out_sock_fd,
			  struct inet6_tx_mc_sock_params tx_sock_parms)
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
		log_debug_low("%s(): errno == %d\n", __func__, errno);
		if (rx_pkt_len > 0) {
			inet6_tx_mcast(*inet6_out_sock_fd, pkt_buf, rx_pkt_len,
				tx_sock_parms.mc_dests,
				tx_sock_parms.mc_dests_num);
		}
	}

	log_debug_med("\t%s() exit\n", __func__);

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


	log_debug_med("%s() entry\n", __func__);

	if (sock_fd != -1) {
		close(sock_fd);
	}

	log_debug_med("%s() exit\n", __func__);

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
