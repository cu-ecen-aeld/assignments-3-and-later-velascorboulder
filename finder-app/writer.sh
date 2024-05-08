#!/bin/bash
if [ $# -eq 2 ]
then
	writeFile=$1
	writeStr=$2
	mkdir -p $(dirname ${writeFile})
	touch ${writeFile}
	$(echo ${writeStr} > ${writeFile})
	if [ ! -f ${writeFile} ]
	then
		echo "Error: File not created"
		exit 1
	fi
else
	echo "Error: There should be two arguments in the command"
	echo "Example: ./writer.sh <file> <string>"
	exit 1
fi

