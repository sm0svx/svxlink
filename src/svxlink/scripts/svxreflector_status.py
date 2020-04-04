#!/usr/bin/env python2

import os
import requests

# API endpoint
URL = "http://localhost:8080/status"

# Colors
GREEN="\033[0;32m"
RED="\033[0;31m"
NC="\033[0m"

# Get SvxReflector status
r = requests.get(url = URL)

# Extract data in JSON format
data = r.json()

# Get terminal size
rows, columns = os.popen('stty size', 'r').read().split()

row_fmt = "%-9s | %7s | %15s | %3s | %s"
print row_fmt % ("Callsign", "TG#", "RX", "Lev", "Monitored TGs")
print "-" * int(columns)
for callsign in sorted(data['nodes'].keys()):
    node = data['nodes'][callsign]
    monitored_tgs = " ".join(str(tg) for tg in node['monitoredTGs'])
    tg = str(node['tg'])
    rx_name = ''
    siglev = ''
    if 'qth' in node:
        for qth in node['qth']:
            if 'rx' in qth:
                for rx_id in qth['rx']:
                    rx = qth['rx'][rx_id]
                    if 'sql_open' in rx and rx['sql_open']:
                        rx_name = rx['name']
                        siglev = rx['siglev']
    color = GREEN
    #print unicode(rx['name']).encode('utf8')
    if node['isTalker']:
        color = RED
    print (color + row_fmt % (callsign, tg, rx_name, siglev, monitored_tgs) + NC).encode('utf-8')
