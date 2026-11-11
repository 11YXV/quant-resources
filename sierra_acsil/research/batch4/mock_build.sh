#!/bin/bash
# build the three mock harnesses; extra flags in $1
M=/home/user/prev/sierra_acsil/research/mock
for f in DailyWashoutRebound ValueAreaReentry CompressionExpansionBreakout; do
  g++ -std=c++17 -O2 -w $1 -I$M/inc -DSTUDY_FILE="\"$PWD/${f}_NQ_MNQ.cpp\"" -DSTUDY_FN=scsf_${f}_NQ_MNQ $M/harness.cpp -o h_$f${2} || exit 1
done
