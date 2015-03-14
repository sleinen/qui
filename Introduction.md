# Motivation #

We were debugging a couple of network performance problems that involved applications receiving data from the network over UDP without flow control.  Examples of such applications include DNS servers or Netflow (over UDP) accounting systems.  The applications in question sometimes missed incoming packets.  This means that the receive buffer of the UDP socket must have been overflowed from time to time.  I wanted to be able to observe these receive buffers to see when it fills up and how much.  So I first wrote a two-line Perl script that parsed /proc/net/udp, decoded the input-queue value of the socket that I was interested in, and wrote it out if it exceeded some threshold.  Due to the usual feature creep, the tool soon added lots of options and rudimentary graphics.

# Application #

When you have an application that sometimes falls behind with processing traffic from the network, you can use this tool to look at the occupancy of the input queue of its receive socket(s) over time, with a relatively high sampling rates (many times per second).

This is useful in several respects:
  * You can learn how big the queue can get, and dimension your socket's receive buffers accordingly to avoid packet drops in most cases.
  * You can learn at which times a queue forms, and try to correlate this with specific actions of your program, or with events elsewhere on your system that can cause your application to block.
  * When looking at multiple queues (for example using the `-a' option), you can see whether losses are correlated between different sockets and/or processes.  This can help you find out whether a bottleneck is specific to one process or more global.