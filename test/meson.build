if not get_option('tests')
  subdir_done()
endif

tests = [
  'crc32',
]

extra_test_sources = files([
])

foreach test_name : tests
  exe = executable(
    test_name + '-test',
    test_name + '.c',
    extra_test_sources,
    link_with : [libdt],
    include_directories : incdir)

  test(
    test_name,
    exe,
    is_parallel : false,
    timeout : 240,
    workdir : meson.source_root())
endforeach

test(
  'barebox-state.t',
  find_program('barebox-state.t'),
  args : '-v',
  is_parallel : false,
  timeout : 300,
  env : ['SHARNESS_BUILD_DIRECTORY=' + meson.build_root()],
  workdir : meson.current_source_dir(),
)
