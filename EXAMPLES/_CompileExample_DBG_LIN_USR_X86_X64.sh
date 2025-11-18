#!/bin/sh

# Args are BUILD_TYPE  APP_NAME  APP_EXT  APP_SRC  OUTPATH  CMPLR_FLAGS
# Use . to skip args if in the middle

SRCPATH="$(dirname "$(realpath "$0")")"	

sh $SRCPATH/FRAMEWORK/COMPILE/COMPILE_BY_CFG.sh DBG_LIN_USR_X86_X64 testapp . . . -mpopcnt -DAPP_MODE

# read ans  # sleep 2