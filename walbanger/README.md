# Harvey the WAL-Banger

## What It Is

Harvey the WAL-Banger is a program that attempts to *disprove* the
proposition that SQLite’s WAL mode is safe across some given boundary.
It was originally written to test the OCI container boundary, since it
is not immediately clear that sharing a Linux kernel allow’s WAL’s
shared memory and locking calls to cooperate across that boundary.

Contrast a network file share, where WAL definitely doesn’t work.  While
this program doesn’t add much to the world knowledge base on that topic,
being a settled question, it does serve as a tool for showing people
inclined to believe this is safe that it is not.

If the program produces no errors, it disproves nothing, but if it blows
up, you’ve shown that WAL is unsafe *in that specific test scenario.*

This awkward language is a simple reflection of the fact that you cannot
prove a negative.  We cannot say “SQLite’s WAL mode will never corrupt
your database when used across a container boundary.”  The best we can
say is that we *failed* to make it corrupt the database in a given test
scenario.


## How to Use It

### Trivial Way

The simplest way to get useful results from this program is to open two
terminal windows in this source directory, then say this in the first:

```shell
$ make run
```

…and from the second:

```shell
$ ./walbanger -p ~/tmp/walbanger.db
```

This runs the same program twice, but the first one is done by a
one-line script that includes the “`-r`” flag to make it reset the DB
before beginning work, while the second one leaves it off so it will
skip that step and go straight to updating the DB as it found it.  If
you say “`make run`” twice instead, the second instance will reset the
first instance’s DB out from under it, which will make it a *sad
program.* 😪 Don’t do that.


### Strong Way

The full-strength version of this test builds a Docker container image
and then instantiates three containers from it, all running
simultaneously on the same machine, with the same mapped-in volume, thus
causing all three instances to be manipulating the same database file.
We use [Docker’s swarm mode][dsm] for this, the simplest way to
orchestrate several containers on a single computer.  The
short-and-sweet is:

```shell
$ docker swarm init              # needed once only
$ make swarm
$ docker service logs -f walbanger
```

That’s it.  It should build the container image and then deploy 3
replicas of it as a service called “`walbanger`” on your swarm manager,
which will be the same as the computer you ran the second command on
unless you already have a swarm set up, and its manager is elsewhere.

(If you have multiple manager nodes in your swarm, Docker will try to
spread the service replicas out round-robin, which isn’t a helpful test:
each node will end up with its own `/db` volume, so you don’t test
whether database sharing works across the container boundary at all.)

The final command tails the logs of all the containers in the swarm.
The idea is, if any of them says anything other than the “Opening”
message, it’s complaining of a problem, likely showing that you’ve got a
problem.


### Tricky Way

Another way you can use this testing tool is in single-container mode
against another copy running out on the host.  In one terminal window,
say:

```she]l
$ make single
```

…and then in another terminal say:

```shell
$ make run
```

The first will run a single container instance with your user’s `~/tmp`
directory mapped into it as `/db`, and the second will run a copy of the
program *outside* the container against the same directory.

This fails on macOS in my testing here, either with SQLite error code 14
(can’t open the database) or 26 (not a database file).  I believe it’s
due to the hidden background VM, since it causes all file I/O to go
through the QEMU soft CPU emulator or through macOS’s internal kernel
hypervisor. I haven’t yet tested on Linux to see if the problem occurs
there, too. If not, it would support this hypothesis.


### Evil Way

Run this program across a network file share. Watch sparks fly.


## Challenge

If you can find a way to make this program fail, reliably, I’d like to
hear about it. I suggest contacting me through [the main SQLite
forum][for1] if the matter is of general interest, or [this repo’s
forum][for2] otherwise.

[dsm]:  https://docs.docker.com/engine/swarm/
[for1]: https://sqlite.org/forum
[for2]: https://tangentsoft.com/sqlite/forum
