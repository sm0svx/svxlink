#!/bin/sh

name=$1
ver=$2

[ -d version ] || mkdir -p version

cat <<-_EOF_ > version/${name}.h
	#ifndef ${name}_VERSION_INCLUDED
	#define ${name}_VERSION_INCLUDED
	
	#define ${name}_VERSION "${ver}"
	
	#endif
	_EOF_

