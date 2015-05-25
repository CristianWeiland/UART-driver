#! /bin/bash

# compile.sh -v main.c && mv prog.bin data.bin .. && (cd .. ; run.sh)

$(cp ./main.c ~/cmips/cMIPS/tests)
$(cp ./handlers.s ~/cmips/cMIPS/include)

exit 0
