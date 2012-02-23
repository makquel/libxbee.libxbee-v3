#ifndef __XBEE_NET_H
#define __XBEE_NET_H

/*
	libxbee - a C library to aid the use of Digi's XBee wireless modules
	          running in API mode (AP=2).

	Copyright (C) 2009	Attie Grande (attie@attie.co.uk)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.	If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

extern struct ll_head *netDeadClientList;

struct xbee_netClientInfo {
	int fd;
	int die;
	
	struct xbee *xbee;
	char addr[INET_ADDRSTRLEN];
	int port;
	
	xsys_thread rxThread;
	xsys_thread rxHandlerThread;
	xsys_thread txThread;
	
	struct xbee_rxInfo *rx;
	struct xbee_txInfo *tx;
	struct xbee_frameBlock *fBlock;
};

struct xbee_netInfo {
	int fd;
	
	xsys_thread serverThread;
	
	struct xbee_netClientInfo *newClient;
	int(*clientFilter)(struct xbee *xbee, char *remoteHost);
	struct ll_head *clientList;
};

xbee_err xbee_netClientAlloc(struct xbee *xbee, struct xbee_netClientInfo **info);
xbee_err xbee_netClientFree(struct xbee_netClientInfo *info);

xbee_err xbee_netClientStartup(struct xbee *xbee, struct xbee_netClientInfo *client);
xbee_err xbee_netClientShutdown(struct xbee_netClientInfo *client);

#endif /* __XBEE_NET_H */
