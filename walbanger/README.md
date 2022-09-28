# About Harvey the WAL-Banger

## What It Is

This tool attempts to *disprove* the
proposition that [SQLite’s WAL mode][WALdoc] is safe across some given boundary.
It was originally written to test the [OCI] container boundary, since it
is not immediately clear that sharing a Linux kernel allows WAL’s
shared memory and locking calls to cooperate across that boundary.

If the program produces no errors, it doesn't prove the test condition
safe; it merely proves that you have *failed to fail* **so far**. The
only thing you can do from the state of lack-of-failure is
to keep trying until you decide that nothing you can do will break it.
Only then can you call the test condition provisionally safe, pending
later proof that it isn't. Such is the nature of epistemology, alas.

You might hope for it to not give an error, but realize that this is the
more interesting condition, since that is a definitive result. If you can
[make it fail in an interesting way](./README.md#failure-modes) you've
learned something definite about how SQLite works under that condition.

We can never say “SQLite’s WAL mode will never corrupt
your database when used in condition X.”  The best we can
say is that we *failed* to make it corrupt the database in that test
scenario. If the test doesn't match reality, or if the test is incomplete,
it will fail to find the corner case that breaks things.

There is [a video](https://vimeo.com/754113094) showing the program in
action, including instructions on how to configure it, run it, and
interpret its output.

[OCI]:    https://opencontainers.org/
[WALdoc]: https://www.sqlite.org/wal.html


## <a id="how"></a>How to Use It

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



## <a id="portability"></a>Portability

The program is written in the C++11 dialect of C++.

Since the current focus of the tool is on testing inter-container
database access, it was written for and tested on POSIX type platforms.

Lack of Windows support is not considered a serious problem. The Alpine
Linux based container built by the included [`Dockerfile`](./Dockerfile)
will run on the Windows version of Docker, and it will of course build and
run under WSL2.


## <a id="results"></a>Results

The most stringent test we've put `walbanger` to so far is running 75 instances
under Docker Swarm on a ten-core macOS 12 box for five hours, then starting a
background script that started killing instances off at random, one per minute.
After nine hours of that, it still hadn't corrupted the database.


## <a id="failure-modes"></a>Known Failure Modes

All of SQLite's "[How To Corrupt An SQLite Database File][sqcor]" warnings
apply, so one of the less valuable things this program offers is a quick
and easy way to trigger these documented failure modes. For instance, running
this program across a network file sharing protocol is expected to fail.
If you've read about that but never saw it happen yourself because you
heeded the warnings, this program provides a quick way to demonstrate
the failure without putting valuable data at risk.


### Split Kernels

One of the less obvious ways we've seen this program fail is to run the
program as a container on the Windows or macOS version of Docker or
Podman, then run a separate copy of the program against the same test
database. It'll fail because you've broken the shared-kernel link that
allows the instances to cooperate in the primary test cases listed
above. Container runtimes on non-Linux OSes must create a hidden
background Linux VM to run the containers under since the host OSes
don't have the kernel primitives needed to run containers themselves.

Contrariwise, it might not fail if you put the database into DELETE mode
before running this test, because that simpler locking model can
cooperate across this boundary, under the right conditions. I've seen it
succeed on macOS, for one. I haven't tested it on Windows, but [I know
enough about the matter][sqlk] from a past life to expect it to *fail*
if the host-side binary is built under Cygwin or you port it to native
Windows via MinGW or Visual C++.

An interesting experiment would be to find out whether it fails when run
as a container under Docker Desktop for Windows while another copy is
running out in WSL2. They *should* share a kernel in this instance, so I
would expect it to succeed. If anyone tries this, I'd be willing to
publish your results.


### A Challenge

If you can find other ways to make this program fail, reliably, I’d like to
hear about it, particularly if you think it's *not expected* to fail.

If you can pull off such a feat, I suggest contacting me through
[the main SQLite forum][for1] if the matter is of general interest, or
[this repo’s forum][for2] otherwise.

[sqcor]: https://www.sqlite.org/howtocorrupt.html


<div id="this-space-left-blank-intentionally" style="height:50em"></div>

[dsm]:  https://docs.docker.com/engine/swarm/
[for1]: https://sqlite.org/forum
[for2]: https://tangentsoft.com/sqlite/forum
[sqlk]: https://stackoverflow.com/a/11887905/142454
