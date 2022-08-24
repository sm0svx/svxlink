#!/usr/bin/perl -w
#
# Example script that uses select() with
# a timeout on a named pipe.
#
use strict;
my $interval=10;  # seconds
my $fifo="/var/log/svxpipe";  # create from shell with "mkfifo"
if (!(-p $fifo && -w _ && -r _))
{
    print STDERR "Bad fifo $fifo!\n";
    exit(1);
}
# Open the fifo read/write, not just read.  This is necessary because
# of the POSIX rules about using select() on named pipes when no writers
# are present.  This is a very key step that is hard to find documentation
# about.
open(FIFOFH,"+<",$fifo) || die $!;

my $rin='';
my $rout='';
vec($rin,fileno(FIFOFH),1)=1;
while(1) {
    my $nfound=select($rout=$rin,undef,undef,$interval);

    if($nfound) {
        my $data;
        while(select($rout=$rin,undef,undef,0))
        {
            $data=<FIFOFH>;
            if ( open(LOG, ">> /var/log/svxlink") ) {
                print LOG scalar(localtime), ": $data";
                close(LOG);
            } else {
                warn "Could not log $data to logfile: $!\n";
            }
        }
    }
}

