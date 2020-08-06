#!/usr/bin/env python3

import requests
import curses
import time
import sys
import traceback

# API endpoint
URL = "http://localhost:8080/status"
if len(sys.argv) > 1:
    URL = sys.argv[1]

def draw_status(stdscr):
    # Get SvxReflector status
    r = requests.get(url = URL)
    if r.status_code != 200:
        stdscr.erase()
        stdscr.addstr(0, 0, '*** ERROR: Could not get ' + URL)
        stdscr.refresh()
        return

    # Extract data in JSON format
    data = r.json()

    # Get terminal size
    rows, columns = stdscr.getmaxyx()

    # Row format for all table rows
    row_fmt = "%-9s | %7s | %15s | %3s | %s"

    # Print header line
    header = row_fmt % ("Callsign", "TG#", "RX", "Lev", "Monitored TGs")
    timestr = time.ctime()
    nspaces = int(columns) - len(timestr) - len(header)
    if nspaces > 0:
        header = header + (' ' * nspaces) + timestr
    stdscr.addstr(0, 0, header, curses.color_pair(3) | curses.A_REVERSE | curses.A_BOLD)

    # Print all node lines
    line = 1
    for callsign in sorted(data['nodes'].keys()):
        node = data['nodes'][callsign]
        monitored_tgs = " ".join(str(tg) for tg in node['monitoredTGs'])
        tg = str(node['tg'])
        rx_name = ''
        siglev = ''
        if 'qth' in node and isinstance(node['qth'], list):
            for qth in node['qth']:
                qth_name = ''
                if 'name' in qth:
                    qth_name = ' ' + qth['name']
                if 'rx' in qth:
                    for rx_id in qth['rx']:
                        rx = qth['rx'][rx_id]
                        if 'active' in rx and rx['active']:
                            rx_name = rx['name'] + qth_name
                            siglev = rx['siglev']
        rx_name = rx_name[0:15]
        attrs = curses.color_pair(2)
        if node['isTalker']:
            attrs = curses.color_pair(1)
        stdscr.addnstr(line, 0, row_fmt % (callsign, tg, rx_name, siglev, monitored_tgs), int(columns), attrs)
        stdscr.clrtoeol()
        line = line + 1
        if line >= int(rows):
            break
    stdscr.clrtobot()
    stdscr.refresh()


def main(stdscr):
    curses.curs_set(False)
    curses.use_default_colors()
    curses.init_pair(1, curses.COLOR_RED, -1)
    curses.init_pair(2, curses.COLOR_GREEN, -1)
    curses.init_pair(3, curses.COLOR_YELLOW, -1)
    stdscr.timeout(500)

    while True:
        try:
            draw_status(stdscr)
        except:
            stdscr.erase()
            stdscr.addstr(0, 0, traceback.format_exc())
            stdscr.refresh()

        # Read command. Will timeout after the time specified above.
        cmd = stdscr.getch()
        if cmd == ord('q'):
            break;


try:
    curses.wrapper(main)
except KeyboardInterrupt:
    pass
