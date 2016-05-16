#!/bin/sh
if [ $# -eq 1 ]; then
	git tag -a $1 -m "Tagging the $1 release"
	git push origin $1
else
	echo "Usage: $0 version"
fi

