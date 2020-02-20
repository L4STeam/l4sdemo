This directory contains the kernel modules used by the testbed,
i.e., all qdiscs and some congestion control algorithms, for various kernel
versions.

More precisely:
- We provide patch on top of common qdisc (red, pie, fq_codel, taildrop), for
multiple kernel versions, that add in the IP ID field the exact queuing delay 
of the packet (in units of 32us, encoded using as a 11b float) as well as a
drop (see the testbed_* functions in common/testbed.h as well as the
(d)encoding function in common/numbers.h)
- We provide tcp_dctcp, patched to react to loss (as it was only backported to
LTS version pre-5.1), as well as with a small tweak on its EWMA computation (i.e.,
we prevent it from resetting its historical term to 0 if the mark rate is low).

If your kernel version is unsupported, you can:
1. Create a new folder with its version number (e.g., 4.17)
2. Copy the content of a previous version into it (e.g., 4.15--to have the basic
directory structure as well as Makefile)
3. Replace all *.c files with their version from a vanilla kernel tree
4. Patch them:
    - Qdisc must call testbed_inc_drop_count() whenever they drop a packet,
    testbed_metrics_init() on init/reset, and testbed_add_metrics() when they
    successfully dequeue a packet before returning it to the caller of 
    qdisc_dequeue().
    - tcp_dctcp must react to losses, see https://patchwork.ozlabs.org/patch/1077231/;
    and its historical EWMA term cannot be reset to zero when updating alpha.
