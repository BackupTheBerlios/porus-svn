#!/bin/bash

# release_export <version> <dir>
#
# export a zippable source dir
# - exports a dir containing exactly those things which go into a source 
#   archive

. ./release_config.sh

sourcedir=$2

echo -n "Getting release directory ... "
svn -q export $release_url $sourcedir 1>/dev/null || {
	echo "Could not get release directory";
	echo "URL: $release_url";
	exit 1;
}
echo "done"

echo -n "Cleaning up release directory ... "
rm -rf $sourcedir/dist
echo "done"

echo "Release directory exported to $sourcedir"
