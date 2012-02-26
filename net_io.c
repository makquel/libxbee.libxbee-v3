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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "internal.h"
#include "xbee_int.h"
#include "mode.h"
#include "net.h"
#include "net_io.h"
#include "ll.h"

/* for the dual-purpose-ness */
#include "modes/net/mode.h"

/* ######################################################################### */

xbee_err xbee_netRx(struct xbee *xbee, void *arg, struct xbee_buf **buf) {
	char c;
	char length[2];
	int pos, len, ret;
	struct xbee_buf *iBuf;
	int fd;
	
	if (!xbee || !buf) return XBEE_EMISSINGPARAM;
	
	if (arg) {
		struct xbee_netClientInfo *info;
		info = arg;
		if (xbee != info->xbee) return XBEE_EINVAL;
		fd = info->fd;
	} else {
		struct xbee_modeData *info;
		info = xbee->modeData;
		fd = info->netInfo.fd;
	}
	
	while (1) {
		do {
			if ((ret = recv(fd, &c, 1, MSG_NOSIGNAL)) < 0) return XBEE_EIO;
			if (ret == 0) goto eof;
		} while (c != 0x7E);
		
		for (len = 2, pos = 0; pos < len; pos += ret) {
			ret = recv(fd, &(length[pos]), len - pos, MSG_NOSIGNAL);
			if (ret > 0) continue;
			if (ret == 0) goto eof;
			return XBEE_EIO;
		}
		
		len = ((length[0] << 8) & 0xFF00) | (length[1] & 0xFF);
		if ((iBuf = malloc(sizeof(*iBuf) + len)) == NULL) return XBEE_ENOMEM;
		ll_add_tail(needsFree, iBuf);
		
		iBuf->len = len;
		
		for (pos = 0; pos < iBuf->len; pos += ret) {
			ret = recv(fd, &(iBuf->data[pos]), iBuf->len - pos, MSG_NOSIGNAL);
			if (ret > 0) continue;
			ll_ext_item(needsFree, iBuf);
			free(iBuf);
			if (ret == 0) goto eof;
			return XBEE_EIO;
		}
		break;
	}
	
	*buf = iBuf;
	
	return XBEE_ENONE;
eof:
	if (arg) {
		struct xbee_netClientInfo *info;
		struct xbee_con *con;
		info = arg;

		/* xbee_netRx() is responsible for free()ing memory and killing off client threads on the server
		   to do this, we need to add ourselves to the netDeadClientList, and remove ourselves from the clientList
		   the server thread will then cleanup any clients on the next accept() */
		ll_add_tail(netDeadClientList, arg);
		ll_ext_item(xbee->netInfo->clientList, arg);

		/* end all of our connections */
		for (con = NULL; ll_ext_head(info->conList, (void **)&con) == XBEE_ENONE && con; ) {
			xbee_conEnd(con);
		}
	}
	return XBEE_EEOF;
}

xbee_err xbee_netTx(struct xbee *xbee, void *arg, struct xbee_buf *buf) {
	int pos, ret;
	int fd;
	size_t txSize;
	size_t memSize;
	struct xbee_buf *iBuf;

	size_t *txBufSize;
	struct xbee_buf **txBuf;
	
	if (!xbee || !buf) return XBEE_EMISSINGPARAM;
	
	if (arg) {
		struct xbee_netClientInfo *info;
		info = arg;
		if (xbee != info->xbee) return XBEE_EINVAL;
		fd = info->fd;

		txBufSize = &info->txBufSize;
		txBuf = &info->txBuf;
	} else {
		struct xbee_modeData *info;
		info = xbee->modeData;
		fd = info->netInfo.fd;

		txBufSize = &info->netInfo.txBufSize;
		txBuf = &info->netInfo.txBuf;
	}
	
	txSize = 3 + buf->len;
	memSize = txSize + sizeof(*iBuf);
	
	iBuf = *txBuf;
	if (!iBuf || *txBufSize < memSize) {
		if ((iBuf = malloc(memSize)) == NULL) return XBEE_ENOMEM;
		*txBuf = iBuf;
		*txBufSize = memSize;
	}
	
	iBuf->len = txSize;
	iBuf->data[0] = 0x7E;
	iBuf->data[1] = ((buf->len) >> 8) & 0xFF;
	iBuf->data[2] = ((buf->len)     ) & 0xFF;
	memcpy(&(iBuf->data[3]), buf->data, buf->len);
	
	for (pos = 0; pos < iBuf->len; pos += ret) {
		ret = send(fd, iBuf->data, iBuf->len - pos, MSG_NOSIGNAL);
		if (ret >= 0) continue;
		return XBEE_EIO;
	}
	
	return XBEE_ENONE;
}