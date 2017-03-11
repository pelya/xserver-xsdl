srcs_xorg_dixmods = [
    'xkbVT.c',
    'xkbPrivate.c',
    'xkbKillSrv.c',
]

xorg_dixmods = static_library('xorg_dixmods',
    srcs_xorg_dixmods,
    include_directories: [inc, xorg_inc],
    dependencies: common_dep,
    c_args: xorg_c_args,
)

shared_module(
    'fb',
    'fbmodule.c',

    include_directories: [inc, xorg_inc],
    c_args: xorg_c_args,
    dependencies: common_dep,
    objects: libxserver_fb.extract_all_objects(),

    install: true,
    install_dir: module_dir,
)

shared_module(
    'shadow',
    'shmodule.c',

    include_directories: [inc, xorg_inc],
    c_args: xorg_c_args,
    dependencies: common_dep,
    objects: libxserver_miext_shadow.extract_all_objects(),

    install: true,
    install_dir: module_dir,
)
