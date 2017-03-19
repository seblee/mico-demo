#
#  UNPUBLISHED PROPRIETARY SOURCE CODE
#  Copyright (c) 2016 MXCHIP Inc.
#
#  The contents of this file may not be disclosed to third parties, copied or
#  duplicated in any form, in whole or in part, without the prior written
#  permission of MXCHIP Corporation.
#


NAME := App_Homekit_Demo

$(NAME)_SOURCES := mico_main.c \
                   HomeKitPairlist.c \
                   HomeKitProfiles.c \
                   HomeKitUserInterface.c
                   
$(NAME)_COMPONENTS := daemons/homekit_server
