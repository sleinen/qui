#!/usr/bin/perl -w
###
### Monitor the (input) queue of a specific UDP socket.
###
### This tool periodically looks at the socket's entry (line) in
### /proc/net/udp, and in particular the input queue.
###
### If the input queue level is greater than some threshold, the tool
### will output a line showing the queue occupancy.

use strict;
use warnings;
use Time::HiRes qw(usleep gettimeofday);
use POSIX qw(strftime);
use FileHandle;
use Getopt::Long;

my $stop = 0;
my %opt = ( threshold => 2000,
	    blipsize => 50000,
	    sleep => 10000,
	  );
my $prefix = '/proc/net';
my @protos = qw/udp udp6 tcp tcp6/;
my (%FH, %stats, $pattern);
my @trackers;

sub monitor_queue($ );
sub blip($$);

die "Usage: $0 [-t threshold] [-b blip_size] [-s sleep_time] [socket ...]\n"
  unless GetOptions(\%opt, 'threshold=i', 'blipsize=i', 'sleep=i');
if (@ARGV) {
  $pattern = "(".join('|',@ARGV).")";
} else {
  $pattern = '(\d+)';
}
for my $proto (@protos) {
  $FH{$proto} = new FileHandle;
  $FH{$proto}->open("<$prefix/$proto") or
    die "Can't open $prefix/$proto: $!";
  while($_ = $FH{$proto}->getline()) {
    next unless /^\s*\d+:/;
    my ($s) = ((split)[9] =~ /$pattern/);
    if (defined $s) {
      print STDERR "Tracking socket $s in $prefix/$proto\n";
      %{$stats{$s}} = ( avg => 0,
			max => 0,
			samples => 0,
		      );
      push(@trackers,
	   sub() {
	     seek($FH{$proto}, 0, 0) or die "Can't seek: $!";
	     while ($_ = $FH{$proto}->getline()) {
	       next unless (my @words = split)[9] eq $s;
	       blip($s, hex((split(':',$words[4]))[1]));
	       last;
	     }
	   });
    }
  }
}

sub blip($$ ) {
  my ($s ,$inq) = @_;

  my $blips = $inq / $opt{blipsize};
  my ($fullblips, $halfblips) = ($blips / 2, $blips % 2);
  my ($sec, $usec) = gettimeofday();
  printf STDOUT ("%s.%06d %10d %9d %s%s\n",
		 strftime("%H:%M:%S", localtime($sec)),
		 $usec,
		 $s,
		 $inq,
		 "X" x $fullblips, ($halfblips ? 'x' : ''))
    if $inq >= $opt{threshold};
  $stats{$s}{avg} = ($stats{$s}{samples}*$stats{$s}{avg} +
		     $inq)/(++$stats{$s}{samples});
  $stats{$s}{max} = $inq if $inq > $stats{$s}{max};
}

$SIG{INT} = sub() { $stop = 1; };
until($stop) {
  foreach my $tracker (@trackers) {
    &{$tracker}();
  }
  usleep($opt{sleep});
}
for my $fh (keys(%FH)) {
  $FH{$fh}->close or die "Can't close file: $!";
}
print STDERR "\nAverage/Maximum queue size per socket\n";
foreach my $s (sort {$a <=> $b} keys(%stats)) {
  printf STDERR ("%08d %8d %8d\n",$s, $stats{$s}{avg},$stats{$s}{max})
      if $stats{$s}{max} >= $opt{threshold};
}
1;
