#!/bin/bash
TOP=$(realpath $(dirname "${BASH_SOURCE[0]}")/..)
DETECTORS_WITH_EXAMPLE=$(ls $TOP/examples)

echo $TOP
echo $DETECTORS_WITH_EXAMPLE

for detector in $DETECTORS_WITH_EXAMPLE
do
    $TOP/rustle $TOP/examples/$detector -d $detector
    # mv $TOP/.tmp/.$detector.tmp $TOP/examples/$detector/expected.txt
    mv ./audit-result/summary.csv $TOP/examples/$detector/expected.csv
done
