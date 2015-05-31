#!/bin/bash

if [ $# -ne 1 ]
then
    echo "./check partition_num"
    exit;
fi


./run_fsck.pl --partition $1 --tmp_dir ./tmp --image disk
