#!/bin/sh

# Copy this fite and _SET_EVARS.sh into your project't root dorectory and run it
# The project directory name must be unique
 
WORKDIR="$(dirname "$(realpath "$0")")" 

PRJNAME="${WORKDIR##*/}"

echo "Project name: " $PRJNAME

# if not inherited 
if [ "$CMPLRGDIR" = "" ]; then
  echo "no compiler path - searcing..."
  if [ -e "$WORKDIR/_SET_EVARS.sh" ]; then 
    echo "found local!"
    . "$WORKDIR/_SET_EVARS.sh"  
  fi 
  if [ "$CMPLRGDIR" = "" ]; then
    if [ -e "$SRCPATH/../TOOLS/_SET_EVARS.sh" ]; then 
      echo "found global!"
      . "$SRCPATH/../TOOLS/_SET_EVARS.sh" 
    fi   
  fi
fi

# Not all systems support 'n' in 'ln -sfn' so the target link must be deleted first

if [ -n "$FWKSRCDIR" ]; then
  mkdir -p "$FWKSRCDIR"
  if [ -e "$WORKDIR/FRAMEWORK" ]; then 
    echo "WARNING: the target link already exists and will be overwritten: " "$WORKDIR/FRAMEWORK"
    rm "$WORKDIR/FRAMEWORK"
  fi
  ln -s "$FWKSRCDIR" "$WORKDIR/FRAMEWORK"
fi


if [ -n "$PRJBUILDDIR" ]; then
  mkdir -p "$PRJBUILDDIR"
  mkdir -p "$WORKDIR/BUILD"
  if [ -e "$PRJBUILDDIR/$PRJNAME" ]; then 
    echo "WARNING: the target link already exists and will be overwritten: " "$PRJBUILDDIR/$PRJNAME"
    rm "$PRJBUILDDIR/$PRJNAME"
  fi
  ln -s "$WORKDIR/BUILD" "$PRJBUILDDIR/$PRJNAME" 
fi


if [ -n "$BACKUPSRCDIR" ]; then
  mkdir -p "$BACKUPSRCDIR"
  if [ -e "$BACKUPSRCDIR/$PRJNAME" ]; then 
    echo "WARNING: the target link already exists and will be overwritten: " "$BACKUPSRCDIR/$PRJNAME"
    rm "$BACKUPSRCDIR/$PRJNAME"
  fi
  ln -s "$WORKDIR" "$BACKUPSRCDIR/$PRJNAME"
fi

echo "The links are created"

# read ans