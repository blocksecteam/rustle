#!/bin/bash
TOP=$(realpath $(dirname "${BASH_SOURCE[0]}")/..)
DETECTORS_WITH_EXAMPLE=$(ls $TOP/examples)

echo $TOP
echo $DETECTORS_WITH_EXAMPLE

for detector in $DETECTORS_WITH_EXAMPLE
do
    $TOP/rustle $TOP/examples/$detector -d $detector
    cmp --silent $TOP/.tmp/.$detector.tmp $TOP/examples/$detector/expected.txt || die "Unit test fails for the $detector." 1
done
