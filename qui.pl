#!/usr/bin/perl -w
###
### Monitor the (input) queue of a specific UDP socket.
###
### This tool periodically looks at the socket's entry (line) in
### /proc/net/udp, and in particular the input queue.
###
### If the input queue level is greater than some threshold, the tool
### will output a line showing the queue occupancy.

use Time::HiRes qw(usleep gettimeofday);
use POSIX qw(strftime);
use Getopt::Long;

sub monitor_queue($ );

my $socket_id = 42532440;

my $threshold = 2000;

my $blipsize = 50000;

my $all_p = 0;

die "Usage: $0 [-s socket_id] [-t threshold]\n"
    unless GetOptions("all", \$all_p,
		      "socket=i", \$socket_id,
		      "threshold=i", \$threshold,
    );

monitor_queue($socket_id);
1;

sub monitor_queue($ ) {
    my ($socket_id) = @_;

    while (1) {
	if (!get_queues($socket_id, $all_p,
			sub () {
			    my ($s, $inq) = @_;
			    if (defined $inq) {
				my $blips = $inq / $blipsize;
				my ($fullblips, $halfblips) = ($blips / 2, $blips % 2);
				my ($sec, $usec) = gettimeofday();
				printf STDOUT ("%s.%06d %10d %9d %s%s\n",
					       strftime("%H:%M:%S", localtime($sec)),
					       $usec,
					       $s,
					       $inq,
					       "X" x $fullblips, ($halfblips ? 'x' : ''))
				    if $inq >= $threshold;
			    }
			})) {
	    warn "Socket not found";
	    return undef;
	}
	usleep(10000);
    }
}

sub get_queues($$$ ) {
    my ($socket_id, $all_p, $closure) = @_;
    my $count = 0;
    my @procfiles = qw(/proc/net/udp /proc/net/udp6 /proc/net/tcp /proc/net/tcp6);
    $socket_id = '\S+' if $all_p;

    foreach my $procfile (@procfiles) {
	open UDP, $procfile or die;
	while (<UDP>) {
	    my ($inq, $s);
	    if ($all_p) {
		($inq, $s) = /^\s*\d+:\s*\S+\s+\S+\s+\S+\s+\S+:(\S+)\s+\S+\s+\S+\s+\S+\s+\S+\s+(\S+)/;
	    } else {
		($inq, $s) = /^\s*\d+:\s*\S+\s+\S+\s+\d+\s+\S+:(\S+)\s+\S+\s+\S+\s+\S+\s+\S+\s+($socket_id)/;
	    }
	    if (defined $inq) {
		++$count;
		$inq = hex($inq);
		&{$closure} ($s, $inq);
	    }
	}
	close UDP or die "Error closing $procfile: $!";
    }
    return $count;
}
