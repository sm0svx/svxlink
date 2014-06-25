#!/usr/bin/perl
#
# @file    nhrcx.pl
# @brief   Perl-script to link a NHRCx controller to SvxLink over
#          Linux pseudo tty's
# @author  Adi Bier / DL1HRC
# @date    2014-05-01
#
# Run this script after starting SvxLink. Do not configure the
# links in the /dev directory.
#
# SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
# Copyright (C) 2004-2014  Tobias Blomberg / SM0SVX
#

use Time::HiRes qw(usleep);
use Device::SerialPort;
use IO::File;

$serialport = "/dev/ttyS0";
$serialbaud = 2400;

# the Pseudo-tty's
$sql_port   = "/tmp/sql";
$dtmf_port  = "/tmp/dtmf";
$ptt_port   = "/tmp/ptt";
$sql_pin    = "CTS";
$logfile    = "/var/log/nhrc-x.log";
$DEBUG      = 1;
################################################################################
#
#       SvxLink             nhrcx.pl                                  Link/Rpt
# |----------------|   |----------------|
# | SQL   /tmp/sql |<--|   Perlskript   |
# |                |   |                |    |------------------|
# | PTT   /tmp/ptt |-->|     /dev/ttyS0 |<-->| NHRCx-Controller |<--> TRX-Ctrl
# |                |   |                |    |------------------|
# | Dtmf /tmp/dtmf |<--|                |
# |                |   |----------------|
# |                |
# | Audio          |
# |       TX  hw:0 |------------------------------------------------> TX-Audio
# |                |
# |       RX  hw:0 |<------------------------------------------------ RX-Audio
# |----------------|
#
################################################################################

# Hash not used for ptty at the moment
%dtmfch = ('D' => 0x10, '1' => 0x11, '2' => 0x12, '3' => 0x13,
           '4' => 0x14, '5' => 0x15, '6' => 0x16, '7' => 0x17,
           '8' => 0x18, '9' => 0x19, '0' => 0x1A, '*' => 0x1B,
           '#' => 0x1B, 'A' => 0x1C, 'B' => 0x1D, 'C' => 0x1E);

$serial = &open_port($serialport, $serialbaud); # serial port to NHRCx
$PTT = openPtty("$ptt_port"); #  ptt port from SvxLink
$DTMF= openPtty("$dtmf_port"); # dtmf port to SvxLink
$SQL = openPtty("$sql_port"); # sql port to SvxLink

while (1) {
  # request the last dtmf character from NHRCx
  $message = "C";

  # read data from serial port to/from NHRCx
  ($cnt, $instr) = $serial->read(1);

  if ($cnt > 0) {
    # dtmf character received?
    if (($instr ne ' ') && ($instr ne 'K') && ($instr ne $last_dtmf)) {
      &writelog("DTMF-Rx: in=>$instr<");
      $DTMF->write($instr);
      $last_dtmf = $instr;
      $dtmfchg = 1;
    }

    if ($instr eq ' ' && $dtmfchg == 1) {
      &writelog("DTMF has changed, old $last_dtmf, new $instr");
      $DTMF->write(chr(0x20));
      $last_dtmf = ' ';
      $dtmfchg = 0;
    }

    if ($instr ne '' && $instr ne ' ') {
      &writelog("INSTR: >$instr< ".ord($instr));
    }
  }

    # receive and send ptt state
  $PTT->read($p, 1);
  if ($p gt ' ') {
    $message = $p;
    &writelog("PTT-command: $p");
    undef $p;
  }

  # check squelch state, if changeing overwrite dtmf request
  $s = ($serial->modemlines & 64);
  if ($s != $last_sql) {
    $r = ($s ? "O" : "Z");
    $last_sql = $s;
    $SQL->write($r);
    &writelog("The SQL is $r");
  }

  # sending data to NHRCx device
  $serial->write($message);
  usleep(10000);
}
close($PTT);
close($serial);
close($DTMF);
close($SQL);
exit;

sub open_port {
  my $sp;

  &writelog("opening $_[0] with $_[1] baud");

  # the port to/from the NHRCx device
  $sp = new Device::SerialPort($_[0])||die "Can not open $_[0]\n";
  $sp->parity("none");
  $sp->handshake("none");
  $sp->baudrate($_[1]);
  $sp->databits(8);
  $sp->stopbits(0);
  unless ($sp->write_settings()) {
    &writelog("Can not set serial port data for $_[0]\n");
    print "Can not set serial port data for $_[0]\n";
    return 0;
  }
  return $sp;
}

sub openPtty {
  my $fh = IO::File->new($_[0], O_NONBLOCK|O_RDWR);
  if (!(defined $fh)) {
    &writelog("Can not open $_[0]");
    print "Can not open $_[0]\n";
    return 0;
  }
  $fh->autoflush(1);
  &writelog("opening $_[0] OK");
  return $fh;
}

sub writelog {
  if ($DEBUG) {
    open(LOG,">>$logfile");
      print LOG $_[0],"\n";
    close(LOG);
  }
}
