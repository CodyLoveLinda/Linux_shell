#!/bin/bash
# Testing string equality
#
#testuser=ming
#
if [ $USER != $1 ]
then
	echo "This is not $1"
else
	echo "Welcome $1"
fi
#
