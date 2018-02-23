#!/bin/bash
#available bandwidth estimation
ISSUE=issue29
SIMUTAG=simu4
SEED=4
DIRECTORY=data/$ISSUE/$SIMUTAG

# SIMUNAME=bn2M-bitrate
# rm -f out.txt
# touch out.txt
if [ ! -d "$DIRECTORY" ]; then
  mkdir "$DIRECTORY"
fi
waf --run='simulation --targetDt=4.0 --ipswindow=1s --Issue='$ISSUE' --Simutag='$SIMUTAG' --Seed='$SEED''

# mv out.txt "$DIRECTORY"/"$SIMUNAME".txt
# mv data/L3Rate.txt "$DIRECTORY"/L3rate.txt
