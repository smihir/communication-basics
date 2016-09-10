# communication-basics
[Project 1: Intro to Communication](http://pages.cs.wisc.edu/~remzi/Classes/739/Fall2016/Projects/p1.html) as part of CS739 in Fall 2016.

## Part 0: Timing
Measure precision of timers [clock_gettime()](http://linux.die.net/man/3/clock_gettime) or [gettimeofday()](http://linux.die.net/man/2/gettimeofday).
 We are using **clock_gettime()** because **gettimeofday()** does not provide a monotonically increasing clock as it can be affected by NTP
 updates or when administrator changes the system time. The timing measurement code is in the *precision* directory.

