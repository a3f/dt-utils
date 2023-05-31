# Copyright 2013-2023 The DT-Utils Authors <oss-tools@pengutronix.de>
# Homepage: https://git.pengutronix.de/cgit/tools/dt-utils

project(
  'dt-utils',
  'c',
  version : '2021.03.0',
  meson_version : '>=0.51',
  default_options: [
    'c_std=gnu11',
    'warning_level=2',
  ],
  license : 'GPL-2.0-only',
)

cc = meson.get_compiler('c')

conf = configuration_data()
# Statically define to be enabled to harmonize barebox' & dt-utils' code base.
conf.set10('CONFIG_MTD', true)
conf.set10('CONFIG_STATE', true)
conf.set10('CONFIG_STATE_BACKWARD_COMPATIBLE', get_option('state-backward-compatibility'))
conf.set10('CONFIG_LOCK_DEVICE_NODE', get_option('lock-device'))

meson.add_dist_script(
  find_program('check-news.sh').path(),
  '@0@'.format(meson.project_version())
)

prefixdir = get_option('prefix')
if not prefixdir.startswith('/')
        error('Prefix is not absolute: "@0@"'.format(prefixdir))
endif
sysconfdir = join_paths(prefixdir, get_option('sysconfdir'))
conf.set('SYSCONFDIR', sysconfdir)
libexecdir = join_paths(prefixdir, get_option('libexecdir'))
conf.set('LIBEXECDIR', libexecdir)

udevdep = dependency('libudev')

c_flags = '''
  -fno-strict-aliasing
  -ffunction-sections
  -fdata-sections
'''.split()

c_warn_flags = '''
  -Wmissing-declarations
  -Wmissing-prototypes
  -Wnested-externs
  -Wno-sign-compare
  -Wno-unused-parameter
  -Wchar-subscripts
  -Wstrict-prototypes
  -Wshadow
  -Wformat-security
  -Wtype-limits
  -Wno-pointer-sign
  -Wno-shadow
'''.split()
c_flags += cc.get_supported_arguments(c_warn_flags)
add_project_arguments(c_flags, language : 'c')
# enable GNU extensions (required!)
add_project_arguments('-D_GNU_SOURCE', language : 'c')

ld_flags = '''
  -Wl,--gc-sections
  -Wl,--as-needed
'''.split()

incdir = include_directories(['src/dt', 'src'])

sources_libdt = files('''
  src/crc32.c
  src/libdt.c
  src/fdt.c
'''.split())

if get_option('barebox-state')
  sources_barebox_state = files('''
    src/crypto/digest.c
    src/crypto/hmac.c
    src/crypto/sha1.c
    src/crypto/sha2.c
    src/keystore-blob.c
    src/base64.c
    src/barebox-state/backend_bucket_circular.c
    src/barebox-state/backend_bucket_direct.c
    src/barebox-state/backend_format_dtb.c
    src/barebox-state/backend_format_raw.c
    src/barebox-state/backend_storage.c
    src/barebox-state/state.c
    src/barebox-state/state_variables.c
    src/barebox-state.c
  '''.split())
endif

sources_dtblint = files('''
  src/dtblint.c
  src/dtblint-imx-pinmux.c
  src/dtblint.h
'''.split())

sources_fdtdump = files('''
src/fdtdump.c
'''.split())

config_h = configure_file(
  output : 'config.h',
  configuration : conf
)
add_project_arguments('-include', meson.current_build_dir() / 'config.h', language : 'c')

version_h = vcs_tag(
  input : 'version.h.in',
  output : 'version.h',
)
versiondep = declare_dependency(sources: version_h)

meson.add_dist_script('version-gen', meson.project_version())

mapfile = 'src/libdt-utils.sym'
libdt_ld_flags = '-Wl,--version-script,@0@/@1@'.format(meson.current_source_dir(), mapfile)

libdt = shared_library('dt-utils',
  sources_libdt,
  include_directories : incdir,
  link_args : ld_flags + ['-Wl,--no-undefined', libdt_ld_flags],
  link_depends : mapfile,
  c_args : ['-include', meson.current_build_dir() / 'version.h'],
  dependencies : [udevdep, versiondep],
  gnu_symbol_visibility : 'default',
  install : true)

executable('barebox-state',
  sources_barebox_state,
  include_directories : incdir,
  link_args : ld_flags,
  c_args : ['-include', meson.current_build_dir() / 'version.h'],
  dependencies : [versiondep],
  link_with : libdt,
  install : true)

executable('fdtdump',
  sources_fdtdump,
  include_directories : incdir,
  link_args : ld_flags,
  c_args : ['-include', meson.current_build_dir() / 'version.h'],
  dependencies : [versiondep],
  link_with : libdt,
  install : true)

executable('dtblint',
  sources_dtblint,
  include_directories : incdir,
  link_args : ld_flags,
  c_args : ['-include', meson.current_build_dir() / 'version.h'],
  dependencies : [versiondep],
  link_with : libdt,
  install : true)
