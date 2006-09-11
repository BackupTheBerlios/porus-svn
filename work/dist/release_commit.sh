#!/bin/bash

# release_commit <version> <workdir> -- commit a release
# - copies the working dir to the release dir
# - checks out the release to <workdir>
# - updates the version numbers in the release (using release_version.sh)
# - commits the release
# - does not delete <workdir>

. ./release_config.sh

relmsg="release.sh [$version]: copied release"
workdir=$2

echo -n "Checking whether release exists in repository ... "
svn_path_exists $release_url && { echo "yes"; echo "Release already exists"; exit 1; }
echo "no"

echo -n "Copying head to $release_url ... "
svn -q copy -m "$relmsg" $repo/$work $release_url || { 
	echo "error";
	echo "Could not branch working dir!";
	exit 1;
}
echo "done"

echo -n "Checking out release to $workdir ... "
svn -q co $release_url $workdir || {
	echo "error"
	echo "Could not check out release"
	exit 1
}
echo "done"

echo -n "Updating versions ... "
./release_version.sh $1 $workdir || {
	echo "error"
	echo "Could not update versions"
	exit 1
}
echo "done"

echo -n "Committing changes ... "
svn -q ci -m "release.sh [$version]: updated version numbers" $workdir || {
	echo "error"
	echo "Could not commit changes"
	exit 1
}
echo "done"

echo "Release directory is available at $workdir"
