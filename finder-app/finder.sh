#!/bin/sh
# Finder app test script
# Author: Ruben Moreno

#variables for runtime args
filesdir=$1
searchstr=$2

#simple conditional checks

#first check for 2 args
if [ $# -lt 2 ]
then
	echo "Incorrect number of args, retry and enter 2."
	exit 1
fi

#Next check that the 1st arg is a dir
if [ -d "$filesdir" ]
then
	#x is num of files in dir and sub dirs
	#y is the num of matches in the files with searchstr
	NUMFILES=$(find "$filesdir" -type f | wc -l)
	NUMMATCH=$(grep -r "$searchstr" "$filesdir" | wc -l)
	echo "The number of files are $NUMFILES and the number of matching lines are $NUMMATCH"
else
	echo "Arg1 needs to be a valid file path, retry"
	exit 1
fi
