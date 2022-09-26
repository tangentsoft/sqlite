# What It Is

Harvey the WAL-banger is a program that attempts to *disprove* the
proposition that SQLite’s WAL mode is safe across an OCI contianer
boundary.  If you get no errors, you disprove nothing, but if it blows
up, you’ve shown that WAL is unsafe *in that specific test scenario.*

This awkward language is a simple reflection of the fact that you cannot
prove a negative.  We cannot say “SQLite’s WAL mode will never corrupt
your database when used across a container boundary.”  The best we can
say is that we *failed* to make it corrupt the database in a given test
scenario.


# How To Use It

## Easy Way

The full-strength version of this test builds a Docker container and
then runs three copies of it simultaneously on the same mapped-in
volume, thus causing all three instances to be manipulating the same
database file.  We use [Docker’s swarm mode][dsm] for this, the simplest
way to orchestrate several containers on a single computer.  The
short-and-sweet is:

```shell
  $ docker swarm init
  $ make swarm
  $ docker service logs -f walbanger
```

That’s it.  It should build the container image and then deploy 3
replicas of it as a service called “`walbanger`” on your swarm manager,
which will be the same as the computer you ran the second command on
unless you already have a swarm set up, and its manager is elsewhere.

The final command tails the logs of all the containers in the swarm.
The idea is, if any of them says anything other than the “Opening”
message, it’s complaining of a problem, likely showing that you’ve got a
problem. In testing here, I’ve most often run into SQLite error codes 14
(can’t open the database) and 26 (not a database file).


## Tricky Way

Another way you can use this testing tool is in single-container mode
against another copy running out on the host.  In one terminal window,
say:

```she]l
  $ make single
```

…and then in another terminal say:

```
  $ make run
```

The first will run a single container instance with your user’s `~/tmp`
directory mapped into it as `/db`, and the second will run a copy of the
program *outside* the container against the same directory.

This fails on macOS in my testing here. I believe it’s due to the hidden
background VM, since it causes all file I/O to go through the QEMU soft
CPU emulator or through macOS’s inernal kernel hypervisor.  I haven’t
yet tested on Linux to see if the problem occurs there, too.  If not, it
would support this hypotheiss.


# Challenge

If you can find a way to make this program fail, reliably, I’d like to
hear about it. I suggest contacting me through the main SQLite forum if
the matter is of general interest, or [this repo’s forum][for]
otherwise.

[dsm]: https://docs.docker.com/engine/swarm/
[for]: https://tangentsoft.com/sqlite/forum
