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

    action dispatch.h
    action glapi.c
    action glapi.h
    action glapioffsets.h
    action glapitable.h
    action glapitemp.h
    action glprocs.h
    action glthread.c
    action glthread.h
}

symlink_mesa_main() {
    src_dir src/mesa/main
    dst_dir mesa/main

    action accum.c
    action accum.h
    action api_arrayelt.c
    action api_arrayelt.h
    action api_eval.h
    action api_loopback.c
    action api_loopback.h
    action api_noop.c
    action api_noop.h
    action api_validate.c
    action api_validate.h
    action arrayobj.c
    action arrayobj.h
    action attrib.c
    action attrib.h
    action bitset.h
    action blend.c
    action blend.h
    action bufferobj.c
    action bufferobj.h
    action buffers.c
    action buffers.h
    action clip.c
    action clip.h
    action colormac.h
    action colortab.c
    action colortab.h
    action config.h
    action context.c
    action context.h
    action convolve.c
    action convolve.h
    action dd.h
    action debug.c
    action debug.h
    action depth.c
    action depth.h
    action depthstencil.c
    action depthstencil.h
    action dlist.c
    action dlist.h
    action drawpix.c
    action drawpix.h
    action enable.c
    action enable.h
    action enums.c
    action enums.h
    action eval.c
    action eval.h
    action execmem.c
    action extensions.c
    action extensions.h
    action fbobject.c
    action fbobject.h
    action feedback.c
    action feedback.h
    action fog.c
    action fog.h
    action framebuffer.c
    action framebuffer.h
    action get.c
    action get.h
    action getstring.c
    action glheader.h
    action hash.c
    action hash.h
    action hint.c
    action hint.h
    action histogram.c
    action histogram.h
    action image.c
    action image.h
    action imports.c
    action imports.h
    action light.c
    action light.h
    action lines.c
    action lines.h
    action macros.h
    action matrix.c
    action matrix.h
    action mm.c
    action mm.h
    action mtypes.h
    action occlude.c
    action occlude.h
    action pixel.c
    action pixel.h
    action points.c
    action points.h
    action polygon.c
    action polygon.h
    action rastpos.c
    action rastpos.h
    action rbadaptors.c
    action rbadaptors.h
    action renderbuffer.c
    action renderbuffer.h
    action simple_list.h
    action state.c
    action state.h
    action stencil.c
    action stencil.h
    action texcompress.c
    action texcompress.h
    action texcompress_fxt1.c
    action texcompress_s3tc.c
    action texenvprogram.c
    action texenvprogram.h
    action texformat.c
    action texformat.h
    action texformat_tmp.h
    action teximage.c
    action teximage.h
    action texobj.c
    action texobj.h
    action texrender.c
    action texrender.h
    action texstate.c
    action texstate.h
    action texstore.c
    action texstore.h
    action varray.c
    action varray.h
    action version.h
    action vsnprintf.c
    action vtxfmt.c
    action vtxfmt.h
    action vtxfmt_tmp.h
}

symlink_mesa_math() {
    src_dir src/mesa/math
    dst_dir mesa/math

    action m_clip_tmp.h
    action m_copy_tmp.h
    action m_debug.h
    action m_debug_clip.c
    action m_debug_norm.c
    action m_debug_util.h
    action m_debug_xform.c
    action m_dotprod_tmp.h
    action m_eval.c
    action m_eval.h
    action m_matrix.c
    action m_matrix.h
    action m_norm_tmp.h
    action m_trans_tmp.h
    action m_translate.c
    action m_translate.h
    action m_vector.c
    action m_vector.h
    action m_xform.c
    action m_xform.h
    action m_xform_tmp.h
    action mathmod.h
}

symlink_mesa_ac() {
    src_dir src/mesa/array_cache
    dst_dir mesa/array_cache

    action ac_context.c
    action ac_context.h
    action ac_import.c
    action acache.h
}

symlink_mesa_swrast() {
    src_dir src/mesa/swrast
    dst_dir mesa/swrast

    action s_aaline.c
    action s_aaline.h
    action s_aalinetemp.h
    action s_aatriangle.c
    action s_aatriangle.h
    action s_aatritemp.h
    action s_accum.c
    action s_accum.h
    action s_alpha.c
    action s_alpha.h
    action s_arbshader.c
    action s_arbshader.h
    action s_atifragshader.c
    action s_atifragshader.h
    action s_bitmap.c
    action s_blend.c
    action s_blend.h
    action s_blit.c
    action s_buffers.c
    action s_context.c
    action s_context.h
    action s_copypix.c
    action s_depth.c
    action s_depth.h
    action s_drawpix.c
    action s_drawpix.h
    action s_feedback.c
    action s_feedback.h
    action s_fog.c
    action s_fog.h
    action s_imaging.c
    action s_lines.c
    action s_lines.h
    action s_linetemp.h
    action s_logic.c
    action s_logic.h
    action s_masking.c
    action s_masking.h
    action s_nvfragprog.c
    action s_nvfragprog.h
    action s_points.c
    action s_points.h
    action s_pointtemp.h
    action s_readpix.c
    action s_span.c
    action s_span.h
    action s_spantemp.h
    action s_stencil.c
    action s_stencil.h
    action s_texcombine.c
    action s_texcombine.h
    action s_texfilter.c
    action s_texfilter.h
    action s_texstore.c
    action s_triangle.c
    action s_triangle.h
    action s_trispan.h
    action s_tritemp.h
    action s_zoom.c
    action s_zoom.h
    action swrast.h
}

symlink_mesa_ss() {
    src_dir src/mesa/swrast_setup
    dst_dir mesa/swrast_setup

    action ss_context.c
    action ss_context.h
    action ss_triangle.c
    action ss_triangle.h
    action ss_tritmp.h
    action ss_vb.h
    action swrast_setup.h
}

symlink_mesa_tnl() {
    src_dir src/mesa/tnl
    dst_dir mesa/tnl

    action t_array_api.c
    action t_array_api.h
    action t_array_import.c
    action t_array_import.h
    action t_context.c
    action t_context.h
    action t_pipeline.c
    action t_pipeline.h
    action t_save_api.c
    action t_save_api.h
    action t_save_loopback.c
    action t_save_playback.c
    action t_vb_arbprogram.c
    action t_vb_arbprogram.h
    action t_vb_arbprogram_sse.c
    action t_vb_arbshader.c
    action t_vb_cliptmp.h
    action t_vb_cull.c
    action t_vb_fog.c
    action t_vb_light.c
    action t_vb_lighttmp.h
    action t_vb_normals.c
    action t_vb_points.c
    action t_vb_program.c
    action t_vb_render.c
    action t_vb_rendertmp.h
    action t_vb_texgen.c
    action t_vb_texmat.c
    action t_vb_vertex.c
    action t_vertex.c
    action t_vertex.h
    action t_vertex_generic.c
    action t_vertex_sse.c
    action t_vp_build.c
    action t_vp_build.h
    action t_vtx_api.c
    action t_vtx_api.h
    action t_vtx_eval.c
    action t_vtx_exec.c
    action t_vtx_generic.c
    action t_vtx_x86.c
    action tnl.h
}

symlink_mesa_shader() {
    src_dir src/mesa/shader
    dst_dir mesa/shader

    action arbprogparse.c
    action arbprogparse.h
    action arbprogram.c
    action arbprogram.h
    action arbprogram_syn.h
    action atifragshader.c
    action atifragshader.h
    action nvfragparse.c
    action nvfragparse.h
    action nvprogram.c
    action nvprogram.h
    action nvvertexec.c
    action nvvertexec.h
    action nvvertparse.c
    action nvvertparse.h
    action program.c
    action program.h
    action program_instruction.h
    action shaderobjects.c
    action shaderobjects.h
    action shaderobjects_3dlabs.c
    action shaderobjects_3dlabs.h
}

symlink_mesa_shader_grammar() {
    src_dir src/mesa/shader/grammar
    dst_dir mesa/shader/grammar

    action grammar.c
    action grammar.h
    action grammar_syn.h
    action grammar_mesa.c
    action grammar_mesa.h
}

symlink_mesa_shader_slang() {
    src_dir src/mesa/shader/slang
    dst_dir mesa/shader/slang

    action slang_analyse.c
    action slang_analyse.h
    action slang_assemble.c
    action slang_assemble.h
    action slang_assemble_assignment.c
    action slang_assemble_assignment.h
    action slang_assemble_conditional.c
    action slang_assemble_conditional.h
    action slang_assemble_constructor.c
    action slang_assemble_constructor.h
    action slang_assemble_typeinfo.c
    action slang_assemble_typeinfo.h
    action slang_compile.c
    action slang_compile.h
    action slang_compile_function.c
    action slang_compile_function.h
    action slang_compile_operation.c
    action slang_compile_operation.h
    action slang_compile_struct.c
    action slang_compile_struct.h
    action slang_compile_variable.c
    action slang_compile_variable.h
    action slang_execute.c
    action slang_execute.h
    action slang_execute_x86.c
    action slang_export.c
    action slang_export.h
    action slang_library_noise.c
    action slang_library_noise.h
    action slang_library_texsample.c
    action slang_library_texsample.h
    action slang_link.c
    action slang_link.h
    action slang_mesa.h
    action slang_preprocess.c
    action slang_preprocess.h
    action slang_storage.c
    action slang_storage.h
    action slang_utility.c
    action slang_utility.h
    action traverse_wrap.h
}

symlink_mesa_shader_slang_library() {
    src_dir src/mesa/shader/slang/library
    dst_dir mesa/shader/slang/library

    action slang_builtin_vec4_gc.h
    action slang_common_builtin_gc.h
    action slang_core_gc.h
    action slang_fragment_builtin_gc.h
    action slang_shader_syn.h
    action slang_pp_directives_syn.h
    action slang_pp_expression_syn.h
    action slang_pp_version_syn.h
    action slang_vertex_builtin_gc.h
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
