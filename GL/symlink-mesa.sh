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

symlink_mesa_glapi() {
    src_dir src/mesa/glapi
    dst_dir mesa/glapi

    for src in $REAL_SRC_DIR/*.c $REAL_SRC_DIR/*.h; do
        action `basename $src`
    done
}

symlink_mesa_main() {
    src_dir src/mesa/main
    dst_dir mesa/main

    for src in $REAL_SRC_DIR/*.c $REAL_SRC_DIR/*.h; do
        action `basename $src`
    done
}

symlink_mesa_math() {
    src_dir src/mesa/math
    dst_dir mesa/math

    for src in $REAL_SRC_DIR/*.c $REAL_SRC_DIR/*.h; do
        action `basename $src`
    done
}

symlink_mesa_ac() {
    src_dir src/mesa/array_cache
    dst_dir mesa/array_cache

    for src in $REAL_SRC_DIR/*.c $REAL_SRC_DIR/*.h; do
        action `basename $src`
    done
}

symlink_mesa_swrast() {
    src_dir src/mesa/swrast
    dst_dir mesa/swrast

    for src in $REAL_SRC_DIR/*.c $REAL_SRC_DIR/*.h; do
        action `basename $src`
    done
}

symlink_mesa_ss() {
    src_dir src/mesa/swrast_setup
    dst_dir mesa/swrast_setup

    for src in $REAL_SRC_DIR/*.c $REAL_SRC_DIR/*.h; do
        action `basename $src`
    done
}

symlink_mesa_tnl() {
    src_dir src/mesa/tnl
    dst_dir mesa/tnl

    for src in $REAL_SRC_DIR/*.c $REAL_SRC_DIR/*.h; do
        action `basename $src`
    done
}

symlink_mesa_shader() {
    src_dir src/mesa/shader
    dst_dir mesa/shader

    for src in $REAL_SRC_DIR/*.c $REAL_SRC_DIR/*.h; do
        action `basename $src`
    done
}

symlink_mesa_shader_grammar() {
    src_dir src/mesa/shader/grammar
    dst_dir mesa/shader/grammar

    for src in $REAL_SRC_DIR/*.c $REAL_SRC_DIR/*.h; do
        action `basename $src`
    done
}

symlink_mesa_shader_slang() {
    src_dir src/mesa/shader/slang
    dst_dir mesa/shader/slang

    for src in $REAL_SRC_DIR/*.c $REAL_SRC_DIR/*.h; do
        action `basename $src`
    done
}

symlink_mesa_shader_slang_library() {
    src_dir src/mesa/shader/slang/library
    dst_dir mesa/shader/slang/library

    for src in $REAL_SRC_DIR/*.c $REAL_SRC_DIR/*.h; do
        action `basename $src`
    done
}        

symlink_mesa_x() {
    src_dir src/mesa/drivers/x11
    dst_dir mesa/X

    # action glxapi.h
    action glxheader.h
    # action realglx.h
    # action xfonts.h
    action xm_api.c
    action xm_buffer.c
    action xm_dd.c
    action xm_line.c
    action xm_span.c
    action xm_tri.c
    action xmesaP.h

    # another hack
    src_dir src/mesa/drivers/common
    dst_dir mesa/X/drivers/common
    action driverfuncs.c
    action driverfuncs.h
}

symlink_mesa_ppc() {
    src_dir src/mesa/ppc
    dst_dir mesa/ppc
}

symlink_mesa_sparc() {
    src_dir src/mesa/sparc
    dst_dir mesa/sparc
}

symlink_mesa_x86() {
    src_dir src/mesa/x86
    dst_dir mesa/x86
}

symlink_mesa_x8664() {
    src_dir src/mesa/x86-64
    dst_dir mesa/x86-64
}

symlink_mesa() {
    symlink_mesa_main
    symlink_mesa_math
    symlink_mesa_ac
    symlink_mesa_swrast
    symlink_mesa_ss
    symlink_mesa_tnl
    symlink_mesa_shader
    symlink_mesa_shader_grammar
    symlink_mesa_shader_slang
    symlink_mesa_shader_slang_library
    symlink_mesa_x
    symlink_mesa_glapi
    symlink_mesa_ppc
    symlink_mesa_sparc
    symlink_mesa_x86
    symlink_mesa_x8664
}

symlink_glx() {
    # this is... unpleasant
    src_dir src/glx/x11
    dst_dir glx

    action indirect_size.h

    src_dir src/mesa/drivers/dri/common

    action glcontextmodes.c
    action glcontextmodes.h

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

    ACTION=$1 EXPLANATION=$2 run_module mesa
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
    echo symlink.sh src-dir dst-dir
    echo src-dir: the xc directory of the monolithic source tree
    echo dst-dir: the modular source tree containing proto, app, lib, ...
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
