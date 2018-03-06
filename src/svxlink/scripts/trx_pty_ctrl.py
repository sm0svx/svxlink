#!/usr/bin/env python2

import serial
import select
import sys


class GadeliusGcom16:
    DEFAULT_RX_FQ   = 145500000
    DEFAULT_TX_FQ   = DEFAULT_RX_FQ
    DEFAULT_TX_PWR  = 0
    DEFAULT_TX_ON   = False

    def __init__(self, ser_dev_name, poller):
        self.ser_dev_name = ser_dev_name
        self.poller = poller
        self.ser = None
        self.reopen_cnt = 0
        self.current_rx_fq = self.DEFAULT_RX_FQ
        self.current_tx_fq = self.DEFAULT_TX_FQ
        self.current_tx_pwr = self.DEFAULT_TX_PWR
        self.current_tx_on = self.DEFAULT_TX_ON
        self._open_serial_port()

    def _open_serial_port(self):
        try:
            self.ser = serial.Serial(self.ser_dev_name)
            self.ser.baudrate = 9600
            self.reopen_cnt = 0
            self.poller.register(self.ser, select.POLLIN)
            self.set_tx_on(self.current_tx_on)
            self.set_tx_pwr(self.current_tx_pwr)
            self.set_rx_fq(self.current_rx_fq)
            self.set_tx_fq(self.current_tx_fq)
        except serial.serialutil.SerialException as e:
            print "*** WARNING: Could not open serial port:", e
            self.ser = None
            self.reopen_cnt = 100

    def close(self):
        if self.ser is not None:
            self.poller.unregister(self.ser)
            self.ser.close()
            self.ser = None

    def reopen(self):
        self.close()
        self.reopen_cnt = 100

    def fileno(self):
        if self.ser is not None:
            return self.ser.fileno()
        else:
            return -1

    def tick(self):
        if self.ser is None:
            if self.reopen_cnt == 1:
                self.reopen_cnt = 0
                self._open_serial_port()
            elif self.reopen_cnt > 1:
                self.reopen_cnt -= 1

    def write(self, cmd):
        sys.stdout.write(">" + cmd)
        if self.ser is not None:
            self.ser.write(cmd)

    def set_rx_fq(self, fq):
        self.current_rx_fq = fq
        fq = float(fq) / 1000.0
        self.write(b'RFQ %.0f\n' % fq)

    def set_tx_fq(self, fq):
        self.current_tx_fq = fq
        fq = float(fq) / 1000.0
        self.write(b'TFQ %.0f\n' % fq)
        pass

    def set_tx_pwr(self, pwr):
        self.current_tx_pwr = pwr
        self.write('PWR %d\n' % pwr)

    def set_rx_mod(self, mod):
        if mod != 'FM':
            print "*** WARNING: This RX can only handle FM " + \
                  "modulation, not " + mod

    def set_tx_mod(self, mod):
        if mod != 'FM':
            print "*** WARNING: This TX can only handle FM " + \
                  "modulation, not " + mod

    def set_tx_on(self, tx_on):
        self.current_tx_on = tx_on
        try:
            if self.ser is not None:
                self.ser.dtr = tx_on
        except IOError as e:
            print "*** WARNING: Failed to wrte to serial device:", e
            self.reopen()

    def handle_input(self):
        line = self.ser.readline()
        sys.stdout.write("<" + line)

    def sql_open(self):
        try:
            if self.ser is not None:
                return self.ser.cd
            else:
                return False
        except IOError as e:
            print "*** WARNING: Lost serial port connection:", e
            self.reopen()


class Pty:
    CMD_IDLE = 0
    CMD_RXFQ = 1
    CMD_TXFQ = 2
    CMD_RXMOD = 3
    CMD_TXMOD = 4

    def __init__(self, pty_path, trx, poller):
        self.pty_path = pty_path
        self.trx = trx
        self.poller = poller
        self.sql_open = False
        self.pty = None
        self.reopen_cnt = None
        self._open_pty()

    def _open_pty(self):
        if self.pty is not None:
            if self.sql_open:
                self.pty.write('Z')
            self.close()
        self.pty_cmd_state = Pty.CMD_IDLE
        self.sql_open = False
        try:
            self.pty = open(self.pty_path, "wb+", buffering=0)
            self.poller.register(self.pty, select.POLLIN)
        except IOError as e:
            print e
            self.pty = None

    def tick(self):
        if self.pty is None:
            if self.reopen_cnt == 1:
                self.reopen_cnt = 0
                self._open_pty()
            elif self.reopen_cnt > 1:
                self.reopen_cnt -= 1
        sql_open = self.trx.sql_open()
        if sql_open != self.sql_open:
            self.sql_open = sql_open
            self.pty.write('O' if sql_open else 'Z')
            print 'The squelch is %s' % ('OPEN' if sql_open else 'CLOSED')

    def fileno(self):
        return self.pty.fileno()

    def close(self):
        if self.pty is not None:
            self.poller.unregister(self.pty)
            self.pty.close()
            self.pty = None

    def handle_input(self):
        ch = self.pty.read(1)
        if len(ch) == 0:
            print "*** WARNING: PTY not connected"
            self.close()
            self.reopen_cnt = 100
        if self.pty_cmd_state == Pty.CMD_IDLE:
            if ch == 'T':
                print("Turning transmitter ON")
                self.trx.set_tx_on(True)
            elif ch == 'R':
                print("Turning transmitter OFF")
                self.trx.set_tx_on(False)
            elif ch == 'F':
                self.fq = ''
                self.pty_cmd_state = Pty.CMD_TXFQ
            elif ch == 'f':
                self.fq = ''
                self.pty_cmd_state = Pty.CMD_RXFQ
            elif ch == 'M':
                self.mod = ''
                self.pty_cmd_state = Pty.CMD_TXMOD
            elif ch == 'm':
                self.mod = ''
                self.pty_cmd_state = Pty.CMD_RXMOD
        elif self.pty_cmd_state == Pty.CMD_RXFQ:
            if ch != ';':
                self.fq += ch
            else:
                print "Set RX FQ to %sHz" % self.fq
                self.trx.set_rx_fq(self.fq)
                self.pty_cmd_state = Pty.CMD_IDLE
        elif self.pty_cmd_state == Pty.CMD_TXFQ:
            if ch != ';':
                self.fq += ch
            else:
                print "Set TX FQ to %sHz" % self.fq
                self.trx.set_tx_fq(self.fq)
                self.pty_cmd_state = Pty.CMD_IDLE
        elif self.pty_cmd_state == Pty.CMD_RXMOD:
            if ch != ';':
                self.mod += ch
            else:
                print "Set RX modulation to %s" % self.mod
                self.trx.set_rx_mod(self.mod)
                self.pty_cmd_state = Pty.CMD_IDLE
        elif self.pty_cmd_state == Pty.CMD_TXMOD:
            if ch != ';':
                self.mod += ch
            else:
                print "Set TX modulation to %s" % self.mod
                self.trx.set_tx_mod(self.mod)
                self.pty_cmd_state = Pty.CMD_IDLE


poller = select.poll()

trx = GadeliusGcom16("/dev/ttyUSB0", poller)

pty = Pty("/tmp/remotetrx-trx2", trx, poller)

while True:
    events = poller.poll(10)
    pty.tick()
    trx.tick()
    for fd,event in events:
        if fd == trx.fileno():
            trx.handle_input()
        elif fd == pty.fileno():
            pty.handle_input()

trx.close()
pty.close()
