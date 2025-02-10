#!/bin/sh

SRCPATH="$(dirname "$(realpath "$0")")"	

sh $SRCPATH/FRAMEWORK/COMPILE/COMPILE_BY_CFG.sh DBG_LIN_USR_X86_X64 testapp . -mpopcnt -DAPP_MODE

# read ans  # sleep 2