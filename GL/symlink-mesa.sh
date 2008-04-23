#!/bin/sh

#
# A script that symlinks source files from Mesa to modular
#
# Author: Soren Sandmann (sandmann@redhat.com) (original)
# adapted for Mesa by Adam Jackson (ajax@nwnk.net)

#
# Things we would like to do
#
#	- Check that all the relevant files exist
#		- AUTHORS, autogen.sh, configure.ac, ...
#	- Check that we have actually linked everything
#		- if a file doesn't need to be linked, then it needs
#		  to be listed as "not-linked"
#	- Compute diffs between all the files (shouldn't be necessary)
#	- possibly check that files are listet in Makefile.am's
#	- Clean target directory of irrelevant files
#

check_destinations () {
    # don't do anything - we are relying on the side
    # effect of dst_dir
    true
}

check_exist() {
    # Check whether $1 exists

    if [ ! -e $1 ] ; then
	error "$1 not found"
    fi
}

delete_existing() {
    # Delete $2

    rm -f $2
}

link_files() {
    # Link $1 to $2

    if [ ! -e $2 ] ; then
	ln -s $1 $2
    fi
}

main() {
    check_args $1 $2

    run check_destinations "Creating destination directories"
    run check_exist "Checking that the source files exist"
    run delete_existing "Deleting existing files"
    run link_files "Linking files"
}

## actual symlinking

symlink_glx() {
    # this is... unpleasant
    src_dir src/glx/x11
    dst_dir glx

    action indirect_size.h
    action glcontextmodes.c
    action glcontextmodes.h
    action indirect_dispatch.c
    action indirect_dispatch.h
    action indirect_dispatch_swap.c
    action indirect_reqsize.c
    action indirect_reqsize.h
    action indirect_size_get.c
    action indirect_size_get.h
    action indirect_table.c

    src_dir src/mesa/glapi

    action glapi.c
    action glthread.c
}

#########
#
#    Helper functions
#
#########

error() {
	echo
	echo \ \ \ error:\ \ \ $1
	exit 1
}

# printing out what's going on
run_module() {
    # $1 module
    # $2 explanation
    echo -n $EXPLANATION for $1 module ...\ 
    symlink_$1
    echo DONE
}

run() {
    # $1 what to do
    # $2 explanation

    ACTION=$1 EXPLANATION=$2 run_module glx
}

src_dir() {
    REAL_SRC_DIR=$SRC_DIR/$1
    if [ ! -d $REAL_SRC_DIR ] ; then
	error "Source directory $REAL_SRC_DIR does not exist"
    fi
}

dst_dir() {
    REAL_DST_DIR=$DST_DIR/$1
    if [ ! -d $REAL_DST_DIR ] ; then
	mkdir -p $REAL_DST_DIR
    fi
}

action() {
    if [ -z $2 ] ; then
	$ACTION	$REAL_SRC_DIR/$1	$REAL_DST_DIR/$1
    else
	$ACTION	$REAL_SRC_DIR/$1	$REAL_DST_DIR/$2
    fi
}

usage() {
    echo symlink-mesa.sh src-dir dst-dir
    echo src-dir: the Mesa source directory
    echo dst-dir: the GL subdirectory of the Xserver modular tree
}

# Check commandline args
check_args() {
    if [ -z $1 ] ; then
	echo Missing source dir
	usage
	exit 1
    fi

    if [ -z $2 ] ; then
	echo Missing destination dir
	usage
	exit 1
    fi
     
    if [ ! -d $1 ] ; then
	echo $1 is not a dir
	usage
	exit 1
    fi

    if [ ! -d $2 ] ; then
	echo $2 is not a dir
	usage
	exit 1
    fi

    if [ $1 = $2 ] ; then
	echo source and destination can\'t be the same
	usage
	exit 1
    fi

    D=`dirname "$relpath"`
    B=`basename "$relpath"`
    abspath="`cd \"$D\" 2>/dev/null && pwd || echo \"$D\"`/$B"

    SRC_DIR=`( cd $1 ; pwd )`
    DST_DIR=`(cd $2 ; pwd )`
}

main $1 $2
