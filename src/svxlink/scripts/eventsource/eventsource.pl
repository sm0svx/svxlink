#!/usr/bin/perl -wT
# eventsource server
# read svxlink state information and distribute to clients in various formats
# Rob Janssen, PE1CHL

use strict;
use IO::Select;
use IO::Socket;
use Fcntl;
use Socket;
use JSON::PP;
use XML::Simple;

my $state_pty ="/dev/shm/state_pty";
my $logfile = "/var/log/eventsource";
my $datafile = "/var/spool/svxlink/state_info/state.log";

my ($sel,$lsn,$pty,$fh,$fh2,$new,$flags,$peer,$port,$name,$buf,$line);
my ($event,$type,$json,$xs,$out);
my (@ready);
my (%clientfhs,%clientnames,%clienttypes,%parsed,%prev,%jsone,%xmle);

sub logline($) {
    my ($line) = (@_);

    if (open(LOG,">>$logfile")) {
	my $datestring = localtime();
	print LOG $datestring,": ",$line,"\n";
	close LOG;
    } else {
	print $line,"\n";
    }
}

sub matchtype($) {
    my $req;

    ($_) = (@_);
    chomp;
    s/\r//;

    return 1 if ($_ eq "");

    if (($req) = m#^GET /(.*) HTTP#i) {
	$req =~ s/\?.*//;
	return 1 if ($req eq "raw");
	return 2 if ($req eq "json");
	return 3 if ($req eq "xml");
    }

    return 0;
}

sub closeclient($) {
    my ($fh) = (@_);

    $sel->remove($fh);
    close($fh);
    delete $clientfhs{$fh};
    delete $clientnames{$fh};
    delete $clienttypes{$fh};
}

sub parse($) {
    my ($line) = (@_);
    my (@fields,@values,$item,$rx,$sql,$lvl,$name,$value);

    (@fields) = split / /,$line;

    $parsed{"time"} = shift @fields;
    $parsed{"event"} = shift @fields;

    if ($parsed{"event"} eq "Voter:sql_state") {
	# parser for voter squelch and siglev information
	foreach $item (@fields) {
	    ($rx,$sql,$lvl) = ($item =~ m/^(.*)([_:*])([+-]\d\d\d)$/) or next;
	    $sql = "closed" if ($sql eq "_");
	    $sql = "open" if ($sql eq ":");
	    $sql = "active" if ($sql eq "*");
	    $parsed{"rx"}{$rx}{"sql"} = $sql;
	    $parsed{"rx"}{$rx}{"lvl"} = $lvl + 0;
	}
    } else {
	# generic parser for events name=value or name:item1:item2:item3
	foreach $item (@fields) {
	    if (scalar(($name,$value) = split /=/,$item,2) == 2) {
		$parsed{$name} = $value;
	    } elsif (scalar((@values) = split /:/,$item) >= 2) {
		$name = shift @values;
		$parsed{$name} = [ @values ];
	    }
	}
    }
}

sub message($) {
    my ($data) = (@_);
    return "data: ".$data."\r\n\r\n";
}

$lsn = IO::Socket::INET->new(Listen => 1, ReuseAddr => 1, LocalPort => 1535);
$sel = IO::Select->new($lsn);

if (!$lsn or !$sel) {
    logline("failed to open listen socket");
    exit 1;
}

$json = JSON::PP->new;
$json->canonical(0);

$xs = XML::Simple->new(NoIndent => 1);

logline("eventsource server started");

$event = "Voter:sql_state";
$jsone{$event} = $xmle{$event} = ":\r\n\r\n";

while (1) {
    while (!open($pty,$state_pty)) {
	sleep 10;
    }

    $sel->add($pty);
    logline("opened $state_pty");

    if (!open(DATAFILE,">>$datafile")) {
	logline("failed to open datafile $datafile");
	exit 1;
    }

SEL:
    for (;;) {
	if (!(@ready = $sel->can_read(60))) {
	    # Repeat last message as keepalive (not for raw output)
	    foreach $fh2 (keys %clientfhs) {
		if (($type = $clienttypes{$fh2}) == 1) {
		    next;
		} elsif ($type == 2) {
		    $out = $jsone{$event};
		} elsif ($type == 3) {
		    $out = $xmle{$event};
		}

		if (!send($clientfhs{$fh2},$out,MSG_NOSIGNAL)) {
		    $name = $clientnames{$fh2};
		    closeclient($clientfhs{$fh2});
		    logline("error on output to $name");
		}
	    }
	    next;
	}

	foreach $fh (@ready) {
	    if ($fh == $pty) {
		# Read from SvxLink (without buffer because of IO::Select)
		sysread($pty,$buf,4096) or last SEL;

		foreach $line (split /\n/,$buf) {
		    # Deduplication of events
		    (undef,$event,$out) = split / /,$line,3;
		    next if (defined($prev{$event}) and $prev{$event} eq $out);
		    $prev{$event} = $out;

		    # Log the event
		    print DATAFILE "$line\n";

		    # Make JSON and XML event records
		    parse($line);
		    $jsone{$event} = message($json->encode(\%parsed));
		    $xmle{$event} = message($xs->XMLout(\%parsed));
		    undef %parsed;

		    # Send to all clients
		    foreach $fh2 (keys %clientfhs) {
			if (($type = $clienttypes{$fh2}) == 1) {
			    $out = "$line\r\n";
			} elsif ($type == 2) {
			    $out = $jsone{$event};
			} elsif ($type == 3) {
			    $out = $xmle{$event};
			}

			if (!send($clientfhs{$fh2},$out,MSG_NOSIGNAL)) {
			    $name = $clientnames{$fh2};
			    closeclient($clientfhs{$fh2});
			    logline("error on output to $name");
			}
		    }
		}
		next;
	    }

	    if ($fh == $lsn) {
		# Create a new socket
		next if (!defined($new = $lsn->accept()));
		$flags = fcntl($new,F_GETFL,0) or next;
		$flags = fcntl($new,F_SETFL,$flags | O_NONBLOCK) or next;
		$new->setsockopt(SOL_SOCKET,SO_KEEPALIVE,1) or next;
		$peer = $new->peername() or next;
		($port,$name) = unpack_sockaddr_in($peer);
		$name = inet_ntoa($name);
		$sel->add($new);
		$clientnames{$new} = $name;
		logline("accepted connection from $name");
		next;
	    }

	    if (defined($clientfhs{$fh})) {
		# Existing client, more input or EOF => end
		$line = <$fh>;
		$name = $clientnames{$fh};
		send($fh,"EOF\r\n",MSG_NOSIGNAL);
		closeclient($fh);
		logline("end output to $name");
		next;
	    }

	    # New client
	    $line = <$fh>;
	    if (!defined($line)) {
		$name = $clientnames{$fh};
		closeclient($fh);
		logline("no initial input from $name");
		next;
	    }

	    if (($type = matchtype($line)) == 0) {
		send($fh,"HTTP/1.1 404 NOT FOUND\r\n\r\n",MSG_NOSIGNAL);
		$name = $clientnames{$fh};
		closeclient($fh);
		logline("unrecognized input from $name");
		next;
	    }

	    $out = "Origin: *";

	    while (defined($line = <$fh>)) {
		chomp($line);
		$line =~ s/\r//;
		$out = $line if ($line =~ /^Origin:/);
	    };

	    if ($type >= 2) {
		send($fh,"HTTP/1.1 200 OK\r\nAccess-Control-Allow-$out\r\nContent-type: text/event-stream; charset=UTF-8\r\n\r\n",MSG_NOSIGNAL);
	    }

	    if ($type == 2) {
		foreach $out (keys %jsone) {
		    send($fh,$jsone{$out},MSG_NOSIGNAL);
		}
	    } elsif ($type == 3) {
		foreach $out (keys %xmle) {
		    send($fh,$xmle{$out},MSG_NOSIGNAL);
		}
	    }

	    $clientfhs{$fh} = $fh;
	    $clienttypes{$fh} = $type;
	    $name = $clientnames{$fh};
	    logline("enabled output $type to $name");
	}
    }

    $sel->remove($pty);
    close($pty);
    logline("closed $state_pty");
    close DATAFILE;
    sleep 5;
}

logline("eventsource server stopped");
exit 0;
