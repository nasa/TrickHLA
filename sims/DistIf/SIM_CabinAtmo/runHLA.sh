#!/bin/sh

# Starts the HLA federation with RUN_FED_1 and RUN_FED_2
./S_main*.exe RUN_FED_1/input.py &
sleep 1
./S_main*.exe RUN_FED_2/input.py &

