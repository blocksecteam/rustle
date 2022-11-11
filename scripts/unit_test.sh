#!/bin/bash
TOP=$(realpath $(dirname "${BASH_SOURCE[0]}")/..)
DETECTORS_WITH_EXAMPLE=$(ls $TOP/examples)

echo $TOP
echo $DETECTORS_WITH_EXAMPLE

die () {
    echo -e "\e[31m$2\e[0m" 1>&2   # ]]
    echo -e "\e[33mYour result:\e[0m"  # ]]
    cat $3
    echo -e "\e[33mExpected result:\e[0m" # ]]
    cat $4
    exit $1
}

for detector in $DETECTORS_WITH_EXAMPLE
do
    $TOP/rustle $TOP/examples/$detector -d $detector
    # cmp --silent $TOP/.tmp/.$detector.tmp $TOP/examples/$detector/expected.txt || die 1 "Unit test fails for the $detector."
    cmp --silent ./audit-result/summary.csv $TOP/examples/$detector/expected.csv \
        || die 1 "Unit test fails for the $detector." ./audit-result/summary.csv $TOP/examples/$detector/expected.csv
done

echo -e "\e[32mUnit test passed!\e[0m" # ]]
