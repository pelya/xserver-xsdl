/* DO NOT EDIT - This file generated automatically by glX_server_table.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2005, 2006
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <inttypes.h>
#include "glxserver.h"
#include "glxext.h"
#include "indirect_dispatch.h"
#include "indirect_reqsize.h"
#include "g_disptab.h"
#include "indirect_table.h"

/*****************************************************************/
/* tree depth = 3 */
static const int_fast16_t Single_dispatch_tree[24] = {
    /* [0] -> opcode range [0, 256], node depth 1 */
    2,
    5,
    13,
    16,
    EMPTY_LEAF,

    /* [5] -> opcode range [0, 64], node depth 2 */
    2,
    LEAF(0),
    LEAF(16),
    10,
    EMPTY_LEAF,

    /* [10] -> opcode range [32, 48], node depth 3 */
    1,
    LEAF(32),
    EMPTY_LEAF,

    /* [13] -> opcode range [64, 128], node depth 2 */
    1,
    EMPTY_LEAF,
    LEAF(40),

    /* [16] -> opcode range [128, 192], node depth 2 */
    2,
    LEAF(72),
    LEAF(88),
    21,
    EMPTY_LEAF,

    /* [21] -> opcode range [160, 176], node depth 3 */
    1,
    LEAF(104),
    EMPTY_LEAF,

};

static const __GLXdispatchSingleProcPtr Single_function_table[112][2] = {
    /* [  0] =     0 */ {NULL, NULL},
    /* [  1] =     1 */ {__glXDisp_Render, __glXDispSwap_Render},
    /* [  2] =     2 */ {__glXDisp_RenderLarge, __glXDispSwap_RenderLarge},
    /* [  3] =     3 */ {__glXDisp_CreateContext, __glXDispSwap_CreateContext},
    /* [  4] =     4 */ {__glXDisp_DestroyContext, __glXDispSwap_DestroyContext},
    /* [  5] =     5 */ {__glXDisp_MakeCurrent, __glXDispSwap_MakeCurrent},
    /* [  6] =     6 */ {__glXDisp_IsDirect, __glXDispSwap_IsDirect},
    /* [  7] =     7 */ {__glXDisp_QueryVersion, __glXDispSwap_QueryVersion},
    /* [  8] =     8 */ {__glXDisp_WaitGL, __glXDispSwap_WaitGL},
    /* [  9] =     9 */ {__glXDisp_WaitX, __glXDispSwap_WaitX},
    /* [ 10] =    10 */ {__glXDisp_CopyContext, __glXDispSwap_CopyContext},
    /* [ 11] =    11 */ {__glXDisp_SwapBuffers, __glXDispSwap_SwapBuffers},
    /* [ 12] =    12 */ {__glXDisp_UseXFont, __glXDispSwap_UseXFont},
    /* [ 13] =    13 */ {__glXDisp_CreateGLXPixmap, __glXDispSwap_CreateGLXPixmap},
    /* [ 14] =    14 */ {__glXDisp_GetVisualConfigs, __glXDispSwap_GetVisualConfigs},
    /* [ 15] =    15 */ {__glXDisp_DestroyGLXPixmap, __glXDispSwap_DestroyGLXPixmap},
    /* [ 16] =    16 */ {__glXDisp_VendorPrivate, __glXDispSwap_VendorPrivate},
    /* [ 17] =    17 */ {__glXDisp_VendorPrivateWithReply, __glXDispSwap_VendorPrivateWithReply},
    /* [ 18] =    18 */ {__glXDisp_QueryExtensionsString, __glXDispSwap_QueryExtensionsString},
    /* [ 19] =    19 */ {__glXDisp_QueryServerString, __glXDispSwap_QueryServerString},
    /* [ 20] =    20 */ {__glXDisp_ClientInfo, __glXDispSwap_ClientInfo},
    /* [ 21] =    21 */ {__glXDisp_GetFBConfigs, __glXDispSwap_GetFBConfigs},
    /* [ 22] =    22 */ {__glXDisp_CreatePixmap, __glXDispSwap_CreatePixmap},
    /* [ 23] =    23 */ {__glXDisp_DestroyPixmap, __glXDispSwap_DestroyPixmap},
    /* [ 24] =    24 */ {__glXDisp_CreateNewContext, __glXDispSwap_CreateNewContext},
    /* [ 25] =    25 */ {__glXDisp_QueryContext, __glXDispSwap_QueryContext},
    /* [ 26] =    26 */ {__glXDisp_MakeContextCurrent, __glXDispSwap_MakeContextCurrent},
    /* [ 27] =    27 */ {__glXDisp_CreatePbuffer, __glXDispSwap_CreatePbuffer},
    /* [ 28] =    28 */ {__glXDisp_DestroyPbuffer, __glXDispSwap_DestroyPbuffer},
    /* [ 29] =    29 */ {__glXDisp_GetDrawableAttributes, __glXDispSwap_GetDrawableAttributes},
    /* [ 30] =    30 */ {__glXDisp_ChangeDrawableAttributes, __glXDispSwap_ChangeDrawableAttributes},
    /* [ 31] =    31 */ {__glXDisp_CreateWindow, __glXDispSwap_CreateWindow},
    /* [ 32] =    32 */ {__glXDisp_DestroyWindow, __glXDispSwap_DestroyWindow},
    /* [ 33] =    33 */ {NULL, NULL},
    /* [ 34] =    34 */ {NULL, NULL},
    /* [ 35] =    35 */ {NULL, NULL},
    /* [ 36] =    36 */ {NULL, NULL},
    /* [ 37] =    37 */ {NULL, NULL},
    /* [ 38] =    38 */ {NULL, NULL},
    /* [ 39] =    39 */ {NULL, NULL},
    /* [ 40] =    96 */ {NULL, NULL},
    /* [ 41] =    97 */ {NULL, NULL},
    /* [ 42] =    98 */ {NULL, NULL},
    /* [ 43] =    99 */ {NULL, NULL},
    /* [ 44] =   100 */ {NULL, NULL},
    /* [ 45] =   101 */ {__glXDisp_NewList, __glXDispSwap_NewList},
    /* [ 46] =   102 */ {__glXDisp_EndList, __glXDispSwap_EndList},
    /* [ 47] =   103 */ {__glXDisp_DeleteLists, __glXDispSwap_DeleteLists},
    /* [ 48] =   104 */ {__glXDisp_GenLists, __glXDispSwap_GenLists},
    /* [ 49] =   105 */ {__glXDisp_FeedbackBuffer, __glXDispSwap_FeedbackBuffer},
    /* [ 50] =   106 */ {__glXDisp_SelectBuffer, __glXDispSwap_SelectBuffer},
    /* [ 51] =   107 */ {__glXDisp_RenderMode, __glXDispSwap_RenderMode},
    /* [ 52] =   108 */ {__glXDisp_Finish, __glXDispSwap_Finish},
    /* [ 53] =   109 */ {__glXDisp_PixelStoref, __glXDispSwap_PixelStoref},
    /* [ 54] =   110 */ {__glXDisp_PixelStorei, __glXDispSwap_PixelStorei},
    /* [ 55] =   111 */ {__glXDisp_ReadPixels, __glXDispSwap_ReadPixels},
    /* [ 56] =   112 */ {__glXDisp_GetBooleanv, __glXDispSwap_GetBooleanv},
    /* [ 57] =   113 */ {__glXDisp_GetClipPlane, __glXDispSwap_GetClipPlane},
    /* [ 58] =   114 */ {__glXDisp_GetDoublev, __glXDispSwap_GetDoublev},
    /* [ 59] =   115 */ {__glXDisp_GetError, __glXDispSwap_GetError},
    /* [ 60] =   116 */ {__glXDisp_GetFloatv, __glXDispSwap_GetFloatv},
    /* [ 61] =   117 */ {__glXDisp_GetIntegerv, __glXDispSwap_GetIntegerv},
    /* [ 62] =   118 */ {__glXDisp_GetLightfv, __glXDispSwap_GetLightfv},
    /* [ 63] =   119 */ {__glXDisp_GetLightiv, __glXDispSwap_GetLightiv},
    /* [ 64] =   120 */ {__glXDisp_GetMapdv, __glXDispSwap_GetMapdv},
    /* [ 65] =   121 */ {__glXDisp_GetMapfv, __glXDispSwap_GetMapfv},
    /* [ 66] =   122 */ {__glXDisp_GetMapiv, __glXDispSwap_GetMapiv},
    /* [ 67] =   123 */ {__glXDisp_GetMaterialfv, __glXDispSwap_GetMaterialfv},
    /* [ 68] =   124 */ {__glXDisp_GetMaterialiv, __glXDispSwap_GetMaterialiv},
    /* [ 69] =   125 */ {__glXDisp_GetPixelMapfv, __glXDispSwap_GetPixelMapfv},
    /* [ 70] =   126 */ {__glXDisp_GetPixelMapuiv, __glXDispSwap_GetPixelMapuiv},
    /* [ 71] =   127 */ {__glXDisp_GetPixelMapusv, __glXDispSwap_GetPixelMapusv},
    /* [ 72] =   128 */ {__glXDisp_GetPolygonStipple, __glXDispSwap_GetPolygonStipple},
    /* [ 73] =   129 */ {__glXDisp_GetString, __glXDispSwap_GetString},
    /* [ 74] =   130 */ {__glXDisp_GetTexEnvfv, __glXDispSwap_GetTexEnvfv},
    /* [ 75] =   131 */ {__glXDisp_GetTexEnviv, __glXDispSwap_GetTexEnviv},
    /* [ 76] =   132 */ {__glXDisp_GetTexGendv, __glXDispSwap_GetTexGendv},
    /* [ 77] =   133 */ {__glXDisp_GetTexGenfv, __glXDispSwap_GetTexGenfv},
    /* [ 78] =   134 */ {__glXDisp_GetTexGeniv, __glXDispSwap_GetTexGeniv},
    /* [ 79] =   135 */ {__glXDisp_GetTexImage, __glXDispSwap_GetTexImage},
    /* [ 80] =   136 */ {__glXDisp_GetTexParameterfv, __glXDispSwap_GetTexParameterfv},
    /* [ 81] =   137 */ {__glXDisp_GetTexParameteriv, __glXDispSwap_GetTexParameteriv},
    /* [ 82] =   138 */ {__glXDisp_GetTexLevelParameterfv, __glXDispSwap_GetTexLevelParameterfv},
    /* [ 83] =   139 */ {__glXDisp_GetTexLevelParameteriv, __glXDispSwap_GetTexLevelParameteriv},
    /* [ 84] =   140 */ {__glXDisp_IsEnabled, __glXDispSwap_IsEnabled},
    /* [ 85] =   141 */ {__glXDisp_IsList, __glXDispSwap_IsList},
    /* [ 86] =   142 */ {__glXDisp_Flush, __glXDispSwap_Flush},
    /* [ 87] =   143 */ {__glXDisp_AreTexturesResident, __glXDispSwap_AreTexturesResident},
    /* [ 88] =   144 */ {NULL, NULL},
    /* [ 89] =   145 */ {__glXDisp_GenTextures, __glXDispSwap_GenTextures},
    /* [ 90] =   146 */ {__glXDisp_IsTexture, __glXDispSwap_IsTexture},
    /* [ 91] =   147 */ {__glXDisp_GetColorTable, __glXDispSwap_GetColorTable},
    /* [ 92] =   148 */ {__glXDisp_GetColorTableParameterfv, __glXDispSwap_GetColorTableParameterfv},
    /* [ 93] =   149 */ {__glXDisp_GetColorTableParameteriv, __glXDispSwap_GetColorTableParameteriv},
    /* [ 94] =   150 */ {__glXDisp_GetConvolutionFilter, __glXDispSwap_GetConvolutionFilter},
    /* [ 95] =   151 */ {__glXDisp_GetConvolutionParameterfv, __glXDispSwap_GetConvolutionParameterfv},
    /* [ 96] =   152 */ {__glXDisp_GetConvolutionParameteriv, __glXDispSwap_GetConvolutionParameteriv},
    /* [ 97] =   153 */ {__glXDisp_GetSeparableFilter, __glXDispSwap_GetSeparableFilter},
    /* [ 98] =   154 */ {__glXDisp_GetHistogram, __glXDispSwap_GetHistogram},
    /* [ 99] =   155 */ {__glXDisp_GetHistogramParameterfv, __glXDispSwap_GetHistogramParameterfv},
    /* [ 100] =   156 */ {__glXDisp_GetHistogramParameteriv, __glXDispSwap_GetHistogramParameteriv},
    /* [ 101] =   157 */ {__glXDisp_GetMinmax, __glXDispSwap_GetMinmax},
    /* [ 102] =   158 */ {__glXDisp_GetMinmaxParameterfv, __glXDispSwap_GetMinmaxParameterfv},
    /* [ 103] =   159 */ {__glXDisp_GetMinmaxParameteriv, __glXDispSwap_GetMinmaxParameteriv},
    /* [ 104] =   160 */ {__glXDisp_GetCompressedTexImageARB, __glXDispSwap_GetCompressedTexImageARB},
    /* [ 105] =   161 */ {__glXDisp_DeleteQueriesARB, __glXDispSwap_DeleteQueriesARB},
    /* [ 106] =   162 */ {__glXDisp_GenQueriesARB, __glXDispSwap_GenQueriesARB},
    /* [ 107] =   163 */ {__glXDisp_IsQueryARB, __glXDispSwap_IsQueryARB},
    /* [ 108] =   164 */ {__glXDisp_GetQueryivARB, __glXDispSwap_GetQueryivARB},
    /* [ 109] =   165 */ {__glXDisp_GetQueryObjectivARB, __glXDispSwap_GetQueryObjectivARB},
    /* [ 110] =   166 */ {__glXDisp_GetQueryObjectuivARB, __glXDispSwap_GetQueryObjectuivARB},
    /* [ 111] =   167 */ {NULL, NULL},
};

const struct __glXDispatchInfo Single_dispatch_info = {
    8,
    Single_dispatch_tree,
    Single_function_table,
    NULL,
    NULL
};

/*****************************************************************/
/* tree depth = 13 */
static const int_fast16_t VendorPriv_dispatch_tree[138] = {
    /* [0] -> opcode range [0, 131072], node depth 1 */
    2,
    5,
    EMPTY_LEAF,
    102,
    EMPTY_LEAF,

    /* [5] -> opcode range [0, 32768], node depth 2 */
    1,
    8,
    EMPTY_LEAF,

    /* [8] -> opcode range [0, 16384], node depth 3 */
    1,
    11,
    EMPTY_LEAF,

    /* [11] -> opcode range [0, 8192], node depth 4 */
    2,
    16,
    EMPTY_LEAF,
    78,
    EMPTY_LEAF,

    /* [16] -> opcode range [0, 2048], node depth 5 */
    2,
    21,
    EMPTY_LEAF,
    39,
    EMPTY_LEAF,

    /* [21] -> opcode range [0, 512], node depth 6 */
    1,
    24,
    EMPTY_LEAF,

    /* [24] -> opcode range [0, 256], node depth 7 */
    1,
    27,
    EMPTY_LEAF,

    /* [27] -> opcode range [0, 128], node depth 8 */
    1,
    30,
    EMPTY_LEAF,

    /* [30] -> opcode range [0, 64], node depth 9 */
    1,
    33,
    EMPTY_LEAF,

    /* [33] -> opcode range [0, 32], node depth 10 */
    1,
    36,
    EMPTY_LEAF,

    /* [36] -> opcode range [0, 16], node depth 11 */
    1,
    EMPTY_LEAF,
    LEAF(0),

    /* [39] -> opcode range [1024, 1536], node depth 6 */
    2,
    44,
    EMPTY_LEAF,
    56,
    67,

    /* [44] -> opcode range [1024, 1152], node depth 7 */
    1,
    47,
    EMPTY_LEAF,

    /* [47] -> opcode range [1024, 1088], node depth 8 */
    1,
    50,
    EMPTY_LEAF,

    /* [50] -> opcode range [1024, 1056], node depth 9 */
    1,
    53,
    EMPTY_LEAF,

    /* [53] -> opcode range [1024, 1040], node depth 10 */
    1,
    LEAF(8),
    EMPTY_LEAF,

    /* [56] -> opcode range [1280, 1408], node depth 7 */
    1,
    59,
    EMPTY_LEAF,

    /* [59] -> opcode range [1280, 1344], node depth 8 */
    1,
    62,
    EMPTY_LEAF,

    /* [62] -> opcode range [1280, 1312], node depth 9 */
    2,
    EMPTY_LEAF,
    LEAF(16),
    LEAF(24),
    LEAF(32),

    /* [67] -> opcode range [1408, 1536], node depth 7 */
    1,
    70,
    EMPTY_LEAF,

    /* [70] -> opcode range [1408, 1472], node depth 8 */
    1,
    73,
    EMPTY_LEAF,

    /* [73] -> opcode range [1408, 1440], node depth 9 */
    2,
    EMPTY_LEAF,
    LEAF(40),
    LEAF(48),
    EMPTY_LEAF,

    /* [78] -> opcode range [4096, 6144], node depth 5 */
    1,
    EMPTY_LEAF,
    81,

    /* [81] -> opcode range [5120, 6144], node depth 6 */
    1,
    84,
    EMPTY_LEAF,

    /* [84] -> opcode range [5120, 5632], node depth 7 */
    1,
    87,
    EMPTY_LEAF,

    /* [87] -> opcode range [5120, 5376], node depth 8 */
    1,
    90,
    EMPTY_LEAF,

    /* [90] -> opcode range [5120, 5248], node depth 9 */
    1,
    93,
    EMPTY_LEAF,

    /* [93] -> opcode range [5120, 5184], node depth 10 */
    1,
    EMPTY_LEAF,
    96,

    /* [96] -> opcode range [5152, 5184], node depth 11 */
    1,
    99,
    EMPTY_LEAF,

    /* [99] -> opcode range [5152, 5168], node depth 12 */
    1,
    LEAF(56),
    EMPTY_LEAF,

    /* [102] -> opcode range [65536, 98304], node depth 2 */
    1,
    105,
    EMPTY_LEAF,

    /* [105] -> opcode range [65536, 81920], node depth 3 */
    1,
    108,
    EMPTY_LEAF,

    /* [108] -> opcode range [65536, 73728], node depth 4 */
    1,
    111,
    EMPTY_LEAF,

    /* [111] -> opcode range [65536, 69632], node depth 5 */
    1,
    114,
    EMPTY_LEAF,

    /* [114] -> opcode range [65536, 67584], node depth 6 */
    1,
    117,
    EMPTY_LEAF,

    /* [117] -> opcode range [65536, 66560], node depth 7 */
    1,
    120,
    EMPTY_LEAF,

    /* [120] -> opcode range [65536, 66048], node depth 8 */
    1,
    123,
    EMPTY_LEAF,

    /* [123] -> opcode range [65536, 65792], node depth 9 */
    1,
    126,
    EMPTY_LEAF,

    /* [126] -> opcode range [65536, 65664], node depth 10 */
    1,
    129,
    EMPTY_LEAF,

    /* [129] -> opcode range [65536, 65600], node depth 11 */
    1,
    132,
    EMPTY_LEAF,

    /* [132] -> opcode range [65536, 65568], node depth 12 */
    1,
    135,
    EMPTY_LEAF,

    /* [135] -> opcode range [65536, 65552], node depth 13 */
    1,
    LEAF(64),
    EMPTY_LEAF,

};

static const __GLXdispatchVendorPrivProcPtr VendorPriv_function_table[72][2] = {
    /* [  0] =     8 */ {NULL, NULL},
    /* [  1] =     9 */ {NULL, NULL},
    /* [  2] =    10 */ {NULL, NULL},
    /* [  3] =    11 */ {__glXDisp_AreTexturesResidentEXT, __glXDispSwap_AreTexturesResidentEXT},
    /* [  4] =    12 */ {__glXDisp_DeleteTextures, __glXDispSwap_DeleteTextures},
    /* [  5] =    13 */ {__glXDisp_GenTexturesEXT, __glXDispSwap_GenTexturesEXT},
    /* [  6] =    14 */ {__glXDisp_IsTextureEXT, __glXDispSwap_IsTextureEXT},
    /* [  7] =    15 */ {NULL, NULL},
    /* [  8] =  1024 */ {__glXDisp_QueryContextInfoEXT, __glXDispSwap_QueryContextInfoEXT},
    /* [  9] =  1025 */ {NULL, NULL},
    /* [ 10] =  1026 */ {NULL, NULL},
    /* [ 11] =  1027 */ {NULL, NULL},
    /* [ 12] =  1028 */ {NULL, NULL},
    /* [ 13] =  1029 */ {NULL, NULL},
    /* [ 14] =  1030 */ {NULL, NULL},
    /* [ 15] =  1031 */ {NULL, NULL},
    /* [ 16] =  1288 */ {NULL, NULL},
    /* [ 17] =  1289 */ {NULL, NULL},
    /* [ 18] =  1290 */ {NULL, NULL},
    /* [ 19] =  1291 */ {NULL, NULL},
    /* [ 20] =  1292 */ {NULL, NULL},
    /* [ 21] =  1293 */ {__glXDisp_AreProgramsResidentNV, __glXDispSwap_AreProgramsResidentNV},
    /* [ 22] =  1294 */ {__glXDisp_DeleteProgramsNV, __glXDispSwap_DeleteProgramsNV},
    /* [ 23] =  1295 */ {__glXDisp_GenProgramsNV, __glXDispSwap_GenProgramsNV},
    /* [ 24] =  1296 */ {__glXDisp_GetProgramEnvParameterfvARB, __glXDispSwap_GetProgramEnvParameterfvARB},
    /* [ 25] =  1297 */ {__glXDisp_GetProgramEnvParameterdvARB, __glXDispSwap_GetProgramEnvParameterdvARB},
    /* [ 26] =  1298 */ {__glXDisp_GetProgramivNV, __glXDispSwap_GetProgramivNV},
    /* [ 27] =  1299 */ {__glXDisp_GetProgramStringNV, __glXDispSwap_GetProgramStringNV},
    /* [ 28] =  1300 */ {__glXDisp_GetTrackMatrixivNV, __glXDispSwap_GetTrackMatrixivNV},
    /* [ 29] =  1301 */ {__glXDisp_GetVertexAttribdvARB, __glXDispSwap_GetVertexAttribdvARB},
    /* [ 30] =  1302 */ {__glXDisp_GetVertexAttribfvNV, __glXDispSwap_GetVertexAttribfvNV},
    /* [ 31] =  1303 */ {__glXDisp_GetVertexAttribivNV, __glXDispSwap_GetVertexAttribivNV},
    /* [ 32] =  1304 */ {__glXDisp_IsProgramNV, __glXDispSwap_IsProgramNV},
    /* [ 33] =  1305 */ {__glXDisp_GetProgramLocalParameterfvARB, __glXDispSwap_GetProgramLocalParameterfvARB},
    /* [ 34] =  1306 */ {__glXDisp_GetProgramLocalParameterdvARB, __glXDispSwap_GetProgramLocalParameterdvARB},
    /* [ 35] =  1307 */ {__glXDisp_GetProgramivARB, __glXDispSwap_GetProgramivARB},
    /* [ 36] =  1308 */ {__glXDisp_GetProgramStringARB, __glXDispSwap_GetProgramStringARB},
    /* [ 37] =  1309 */ {NULL, NULL},
    /* [ 38] =  1310 */ {__glXDisp_GetProgramNamedParameterfvNV, __glXDispSwap_GetProgramNamedParameterfvNV},
    /* [ 39] =  1311 */ {__glXDisp_GetProgramNamedParameterdvNV, __glXDispSwap_GetProgramNamedParameterdvNV},
    /* [ 40] =  1416 */ {NULL, NULL},
    /* [ 41] =  1417 */ {NULL, NULL},
    /* [ 42] =  1418 */ {NULL, NULL},
    /* [ 43] =  1419 */ {NULL, NULL},
    /* [ 44] =  1420 */ {NULL, NULL},
    /* [ 45] =  1421 */ {NULL, NULL},
    /* [ 46] =  1422 */ {__glXDisp_IsRenderbufferEXT, __glXDispSwap_IsRenderbufferEXT},
    /* [ 47] =  1423 */ {__glXDisp_GenRenderbuffersEXT, __glXDispSwap_GenRenderbuffersEXT},
    /* [ 48] =  1424 */ {__glXDisp_GetRenderbufferParameterivEXT, __glXDispSwap_GetRenderbufferParameterivEXT},
    /* [ 49] =  1425 */ {__glXDisp_IsFramebufferEXT, __glXDispSwap_IsFramebufferEXT},
    /* [ 50] =  1426 */ {__glXDisp_GenFramebuffersEXT, __glXDispSwap_GenFramebuffersEXT},
    /* [ 51] =  1427 */ {__glXDisp_CheckFramebufferStatusEXT, __glXDispSwap_CheckFramebufferStatusEXT},
    /* [ 52] =  1428 */ {__glXDisp_GetFramebufferAttachmentParameterivEXT, __glXDispSwap_GetFramebufferAttachmentParameterivEXT},
    /* [ 53] =  1429 */ {NULL, NULL},
    /* [ 54] =  1430 */ {NULL, NULL},
    /* [ 55] =  1431 */ {NULL, NULL},
    /* [ 56] =  5152 */ {__glXDisp_BindTexImageEXT, __glXDispSwap_BindTexImageEXT},
    /* [ 57] =  5153 */ {__glXDisp_ReleaseTexImageEXT, __glXDispSwap_ReleaseTexImageEXT},
    /* [ 58] =  5154 */ {__glXDisp_CopySubBufferMESA, __glXDispSwap_CopySubBufferMESA},
    /* [ 59] =  5155 */ {NULL, NULL},
    /* [ 60] =  5156 */ {NULL, NULL},
    /* [ 61] =  5157 */ {NULL, NULL},
    /* [ 62] =  5158 */ {NULL, NULL},
    /* [ 63] =  5159 */ {NULL, NULL},
    /* [ 64] = 65536 */ {NULL, NULL},
    /* [ 65] = 65537 */ {__glXDisp_MakeCurrentReadSGI, __glXDispSwap_MakeCurrentReadSGI},
    /* [ 66] = 65538 */ {NULL, NULL},
    /* [ 67] = 65539 */ {NULL, NULL},
    /* [ 68] = 65540 */ {__glXDisp_GetFBConfigsSGIX, __glXDispSwap_GetFBConfigsSGIX},
    /* [ 69] = 65541 */ {__glXDisp_CreateContextWithConfigSGIX, __glXDispSwap_CreateContextWithConfigSGIX},
    /* [ 70] = 65542 */ {__glXDisp_CreateGLXPixmapWithConfigSGIX, __glXDispSwap_CreateGLXPixmapWithConfigSGIX},
    /* [ 71] = 65543 */ {NULL, NULL},
};

const struct __glXDispatchInfo VendorPriv_dispatch_info = {
    17,
    VendorPriv_dispatch_tree,
    VendorPriv_function_table,
    NULL,
    NULL
};

