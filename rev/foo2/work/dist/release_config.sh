repo="https://svn.berlios.de/svnroot/repos/porus"
work="work"
release="rev"
version=$1
sourcename="porus-$1"
release_url=$repo/$release/$1
#tmpdir=`mktemp -d -t ${sourcename}.XXXX` || exit 1
#logfile=$tmpdir/log

svn_path_exists() {
	TMP=`mktemp -t release.XXXX` || {
		echo "can't make temp file!"
		return 1
	}
	svn ls `dirname $1` >$TMP || {
		echo "Failed to list directory"
		rm -f $TMP
		return 2
	}
	grep "^`basename $1`$" $TMP >/dev/null
	ret=$?
	rm -f $TMP
	return $ret
}

replace_python_version() {
	sed -i -e "s/^VERSION='.*'/VERSION='$1'/g" $2
}
