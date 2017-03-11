srcs_os = [
    'WaitFor.c',
    'access.c',
    'auth.c',
    'backtrace.c',
    'client.c',
    'connection.c',
    'inputthread.c',
    'io.c',
    'mitauth.c',
    'oscolor.c',
    'osinit.c',
    'ospoll.c',
    'utils.c',
    'xdmauth.c',
    'xsha1.c',
    'xstrans.c',
    'xprintf.c',
    'log.c',
]

# Wrapper code for missing C library functions
if not cc.has_function('reallocarray')
    srcs_os += 'reallocarray.c'
endif
if not cc.has_function('strcasecmp')
    srcs_os += 'strcasecmp.c'
endif
if not cc.has_function('strcasestr')
    srcs_os += 'strcasestr.c'
endif
if not cc.has_function('strlcat')
    srcs_os += 'strlcat.c'
endif
if not cc.has_function('strlcpy')
    srcs_os += 'strlcpy.c'
endif
if not cc.has_function('strndup')
    srcs_os += 'strndup.c'
endif
if not cc.has_function('timingsafe_memcmp')
    srcs_os += 'timingsafe_memcmp.c'
endif

if cc.has_function('poll')
    srcs_os += 'xserver_poll.c'
endif
if cc.has_function('sigaction')
    srcs_os += 'busfault.c'
endif

if get_option('xdmcp')
    srcs_os += 'xdmcp.c'
endif

libxserver_os = static_library('libxserver_os',
    srcs_os,
    include_directories: inc,
    dependencies: [
        common_dep,
        dl_dep,
        sha1_dep,
        dependency('xau')
    ],
)
