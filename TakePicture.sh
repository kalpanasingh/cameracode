#!/bin/bash

export LD_LIBRARY_PATH=`pwd`

for i in {1..2}

do
  python capture_script.py -r

done

