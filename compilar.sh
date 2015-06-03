#! /bin/bash

compile.sh -v main.c && mv prog.bin data.bin .. && (cd .. ; run.sh)

exit 0
