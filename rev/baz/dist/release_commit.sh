#!/bin/bash

# release_commit <version> <workdir> -- commit a release
# - copies the working dir to the release dir
# - checks out the release to <workdir>
# - updates the version numbers in the release (using release_version.sh)
# - commits the release
# - does not delete <workdir>

. ./release_config.sh

relmsg="release.sh [$version]: copied release"

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

echo -n "Checking out release to $2 ..."
svn -q co $release_url $2
echo "done"

echo -n "Updating versions ..."
./release_version $1 $2
echo "done"

echo -n "Committing changes ..."
svn -q ci -m "release.sh [$version]: updated version numbers" $2
echo "done"

echo "Release directory is available at $2"
