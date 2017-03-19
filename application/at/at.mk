#
#  UNPUBLISHED PROPRIETARY SOURCE CODE
#  Copyright (c) 2016 MXCHIP Inc.
#
#  The contents of this file may not be disclosed to third parties, copied or
#  duplicated in any form, in whole or in part, without the prior written
#  permission of MXCHIP Corporation.
#

NAME := App_At



$(NAME)_INCLUDES := . \
					auto_config \
					http_server \
					protocol \
					Utils \
					Utils/libmicrohttpd \
					../../MiCO/system
					

$(NAME)_SOURCES := auto_config/AutoConfig.c \
				   auto_config/WPS.c \
				   http_server/fatfs.c \
				   http_server/fsdata.c \
				   http_server/http_server_fs_handle.c \
				   http_server/http_server_ota.c \
				   http_server/http_server.c \
				   protocol/LocalTcpServer.c \
				   protocol/MICOBonjour.c \
				   protocol/protocol.c \
				   protocol/RemoteTcpClient.c \
				   protocol/sta_pre.c \
				   protocol/UdpBoardCast.c \
				   protocol/UdpUniCast.c \
				   Utils/libmicrohttpd/internal.c \
				   Utils/libmicrohttpd/postprocessor.c \
				   Utils/http_replace.c \
				   Utils/strlib.c \
				   app_init.c \
				   at_cmd.c \
				   MICOAppEntrance.c \
				   para_config.c \
                   SppProtocol.c \
                   UartRecv.c
                   
$(NAME)_COMPONENTS += daemons/http_server

