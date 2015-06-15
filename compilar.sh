#! /bin/bash

compile.sh -v main.c && mv prog.bin data.bin .. && (cd .. ; run.sh -t 1000)

exit 0
