#!/usr/bin/python3

import dbm
import sys
import os

os.umask(0o007)
with dbm.open('/var/www/genpw/db/svxreflector-genpw', 'c', 0o660) as db:
    if len(sys.argv) > 1:
        callsign = sys.argv[1]
        print(callsign + ': "' + db[callsign].decode() + '"')
        del db[callsign]
    else:
        for k in db.keys():
            print(k.decode() + ': ' + db[k].decode())
