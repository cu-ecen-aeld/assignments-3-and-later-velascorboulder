#!/bin/sh
if [ $# -eq 2 ]
then
	if [ -d $1 ]
	then
		filesdir=$1
		searchstr=$2
		numLines=$(grep ${searchstr} ${filesdir}/* | wc -l)
		numFiles=$(grep -l ${searchstr} ${filesdir}/* | wc -l)
		echo "The number of files are ${numFiles} and the number of matching lines are ${numLines}"
	else
		echo "Error: Directory does not exist"
		exit 1
	fi
else
	echo "Error: There should be two arguments in the command"
	echo "Example: ./finder.sh <directory> <search string>"
	exit 1
fi
