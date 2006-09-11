#!/bin/bash

# release_version <version> <dir>
#
# change the version strings in a PORUS working directory.

. ./release_config.sh

replace_python_version $1 $2/usbgen/setup.py
replace_python_version $1 $2/usbgen/usbgen
echo $1 >$2/VERSION
