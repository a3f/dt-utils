#!/bin/sh

set -e

test_description="barebox-state binary tests"

. ./sharness.sh

# Prerequisite: device tree compiler available [DTC]
dtc --version &&
  test_set_prereq DTC

# Prerequisite: fdtget available [FDTGET]
fdtget --version &&
  test_set_prereq FDTGET

# Prerequisite: C preprocessor available [CPP]
cpp --version &&
  test_set_prereq CPP

# Prerequisite: UUID generator available [UUIDGEN]
uuidgen --version &&
  test_set_prereq UUIDGEN

# Prerequisite: genimage available [GENIMAGE]
genimage --version &&
  test_set_prereq GENIMAGE

# Prerequisite: root available [ROOT]
whoami | grep -q root &&
  test_set_prereq ROOT

# Prerequisite: root available [LOSETUP]
losetup --version  &&
  test_set_prereq LOSETUP

# Prerequisite: udisksctl available [UDISKSCTL]
udisksctl help &&
  test_set_prereq UDISKSCTL

if ! test_have_prereq DTC; then
  skip_all='skipping all tests, dtc not available'
  test_done
fi

if ! test_have_prereq CPP; then
  skip_all='skipping all tests, cpp not available'
  test_done
fi

if ! test_have_prereq GENIMAGE; then
  skip_all='skipping all tests, genimage not available'
  test_done
fi

test_expect_success "barebox-state invalid arg" "
  test_must_fail barebox-state --foobar baz
"

test_expect_success "barebox-state missing arg" "
  test_expect_code 1 barebox-state --get &&
  test_expect_code 1 barebox-state --set &&
  test_expect_code 1 barebox-state --name &&
  test_expect_code 1 barebox-state --input &&
  test_expect_code 1 barebox-state info
"

test_expect_success "barebox-state version" "
  barebox-state --version
"

test_expect_success "barebox-state help" "
  barebox-state --help
"

if test_have_prereq ROOT && test_have_prereq LOSETUP; then
  loopsetup  () { losetup -P --find --show "$1"; }
  loopdetach () { losetup --detach "$1"; }
  losetup=losetup
elif test_have_prereq UDISKSCTL; then
  loopsetup  () {
    output=$(udisksctl loop-setup --file "$1")
    echo $output | sed -n 's/^Mapped file .* as \(.*\)\.$/\1/p'
  }
  loopdetach () { udisksctl loop-delete -b "$1"; }
  losetup=udiskctl
else
  loopsetup  () { :; }
  loopdetach () { :; }
fi

# Prerequisite: loop mount possible [LOOP]
[ -n "$losetup" ] &&
  test_set_prereq LOOP

TEST_TMPDIR=$(mktemp -d)

if test_have_prereq UUIDGEN; then
  partuuid="$(uuidgen)"
  diskuuid="$(uuidgen)"
else
  partuuid="1da8efb0-4932-4786-b1fc-50dd3cff212f"
  diskuuid="30b8d4b2-ec91-4288-9233-cdb643cb5486"
fi

for config in ${SHARNESS_TEST_DIRECTORY}/*.config; do
  basename=$(basename $config .config)
  config_pp=${TEST_TMPDIR}/${basename}.pp.config
  cpp -o${config_pp} -nostdinc -undef -x assembler-with-cpp \
    -D__IMAGE__='"'$basename'.hdimg"' \
    -D__GPT_LOOPDEV_PARTUUID__='"'$partuuid'"' \
    -D__GPT_LOOPDEV_DISKUUID__='"'$diskuuid'"' $config

  genimage --config=${config_pp} --tmppath="${TEST_TMPDIR}/genimage" \
    --inputpath="${TEST_TMPDIR}" --outputpath="${TEST_TMPDIR}" \
    --rootpath="${SHARNESS_TEST_DIRECTORY}"
done

gptloop=$(loopsetup ${TEST_TMPDIR}/gpt.hdimg)
gptnouuidloop=$(loopsetup ${TEST_TMPDIR}/gpt-no-typeuuid.hdimg)
rawloop=$(loopsetup ${TEST_TMPDIR}/raw.hdimg)

for dts in ${SHARNESS_TEST_DIRECTORY}/*.dts; do
  dtb=$(basename ${dts} .dts).dtb
  cpp -nostdinc -undef -D__DTS__ -x assembler-with-cpp \
    -D__GPT_LOOPDEV__='"'$gptloop'"' -D__RAW_LOOPDEV__='"'$rawloop'"' \
    -D__GPT_NO_TYPEUUID_LOOPDEV__='"'$gptnouuidloop'"' \
    -D__GPT_LOOPDEV_PARTUUID__='"'$partuuid'"' \
    -D__GPT_LOOPDEV_DISKUUID__='"'$diskuuid'"' $dts | \
    dtc -O dtb -o "${TEST_TMPDIR}/${dtb}" -b 0

  if [ "${dtb#*-fail.dtb}" != "$dtb" ]; then
    test_expect_success LOOP "barebox-state -i ${dtb} --dump" "
      test_must_fail barebox-state --input ${TEST_TMPDIR}/$dtb --dump
    "
    continue
  fi

  test_expect_success LOOP "barebox-state -i ${dtb} --dump" "
    barebox-state --input ${TEST_TMPDIR}/$dtb --dump
  "

  if test_have_prereq FDTGET && test_have_prereq LOOP; then
    resolution=$(barebox-state --input ${TEST_TMPDIR}/$dtb -vv --dump 2>&1 | \
      grep 'state: backend resolved to ')
    partinfo=$(echo $resolution | sed 's/\/state: backend resolved to //gp')

    set $partinfo
    actual_dev=$1
    actual_off=$2
    actual_siz=$3

    expected_fail=$(fdtget -t s ${TEST_TMPDIR}/${dtb} / expected-dev)
    expected_disk=$(fdtget -t s ${TEST_TMPDIR}/${dtb} / expected-dev)
    expected_partno=$(fdtget -t u ${TEST_TMPDIR}/${dtb} / expected-partno)
    if [ "$expected_partno" != "0" ]; then
      expected_dev="${expected_disk}p${expected_partno}"
    else
      expected_dev="${expected_disk}"
    fi
    expected_off=$(fdtget -t u ${TEST_TMPDIR}/${dtb} / expected-offset)
    expected_siz=$(fdtget -t u ${TEST_TMPDIR}/${dtb} / expected-size)

    test_expect_success LOOP "check backend partition device for ${dtb}" "
      [ "$actual_dev" = "$expected_dev" ]
    "

    test_expect_success LOOP "check backend partition offset for ${dtb}" "
      [ "$actual_off" = "$expected_off" ]
    "

    test_expect_success LOOP "check backend partition size for ${dtb}" "
      [ "$actual_siz" = "$expected_siz" ]
    "
  fi

  test_expect_success LOOP "barebox-state -i ${dtb} --dump-shell" "
    eval $(barebox-state --input ${TEST_TMPDIR}/$dtb --dump-shell)
    [ $? -eq 0 ] || return 1
  "

  test_expect_success LOOP "verify shell dump for ${dtb}" "
    [ $state_bootstate_system0_remaining_attempts = 3 ] || return 2
    [ $state_bootstate_system0_priority = 20 ] || return 2
    [ $state_bootstate_system1_remaining_attempts = 3 ] || return 2
    [ $state_bootstate_system1_priority = 20 ] || return 2
  "

  test_expect_success LOOP "barebox-state -i ${dtb} --set bootstate.last_chosen=0" "
    barebox-state --input ${TEST_TMPDIR}/$dtb --set bootstate.last_chosen=0
  "

  test_expect_success LOOP "barebox-state -i ${dtb} --dump-shell #2" "
    eval $(barebox-state --input ${TEST_TMPDIR}/$dtb --dump-shell)
    [ $? -eq 0 ] || return 1
  "

  test_expect_success LOOP "verify set for ${dtb} #1" "
    [ $state_bootstate_last_chosen = 0 ] || return 2
  "

  test_expect_success LOOP "barebox-state -i ${dtb} --set bootstate.last_chosen=1337" "
    barebox-state --input ${TEST_TMPDIR}/$dtb --set bootstate.last_chosen=1337
  "

  test_expect_success LOOP "barebox-state -i ${dtb} --dump-shell #2" "
    eval $(barebox-state --input ${TEST_TMPDIR}/$dtb --dump-shell)
    [ $? -eq 0 ] || return 1
  "

  test_expect_success LOOP "verify set for ${dtb} #2" "
    #test_when_finished rm -f ${TEST_TMPDIR}/${dtb}
    [ $state_bootstate_system0_remaining_attempts = 3 ] || return 2
    [ $state_bootstate_system0_priority = 20 ] || return 2
    [ $state_bootstate_system1_remaining_attempts = 3 ] || return 2
    [ $state_bootstate_system1_priority = 20 ] || return 2
    [ $state_bootstate_last_chosen = 1337 ] || return 2
  "
done

loopdetach $rawloop
loopdetach $gptnouuidloop
loopdetach $gptloop

rm -rf $TEST_TMPDIR

test_done

# vi: ft=sh
