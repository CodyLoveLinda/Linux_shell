#!/bin/bash
# Testing nested ifs - use elif
#
#testuser=ming
#
if grep $1 /etc/passwd
then
	echo "This user $1 exits on this system."
#
elif ls -d /home/$1
then
	echo "This user $1 does not exist on this system."
	echo "However, $1 has a directory."
else
	echo "NO user $1, and no dirtory"
fi

