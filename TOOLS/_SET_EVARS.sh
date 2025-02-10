#!/bin/sh

# Copy this file into '/etc/profile.d/'
# sudo cp _SET_EVARS.sh /etc/profile.d/

export CMPLRGDIR="/usr/lib/llvm-19"
export FWKSRCDIR="$HOME/_SYNC/COMMONSRC/FRAMEWORK"
export ACTPRJDIR="$HOME/_SYNC/ACTIVEPRJ"

export PRJBUILDDIR="$HOME/_BUILD"
export BACKUPSRCDIR="$HOME/_BACKUPSRC"

echo "vars are set"

# read ans