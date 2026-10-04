# virt-jail

A set of utilities for sandboxing virtual machine emulators on linux machines.

## Work in progress

This repository is currently a *pre-alpha work in progress*, and not yet ready
for use.

## Motivation

Virtual machine emulators such as [QEMU](https://www.qemu.org/) by default
often favor compatibility and usability over security. It is very easy to get
almost anything that will run on a typical PC running on QEMU, from an old MS
DOS game to Windows 95 to a modern BSD server OS, and for many use cases this
set of priorities is a good thing. In some cases, however, you want to run
either an untrusted workload (e.g. one provided by an untrusted tenant) or a
service exposed to the internet, and you want to make sure that a compromise
within the virtual machine can't easy be turned into a compromise of the host
machine. In this case, [the QEMU defaults may not stop a modern attacker](
https://blog.trailofbits.com/2026/08/26/vms-wont-contain-cyber-capable-agents/)
from easily escaping from the guest to the host machine.

QEMU does provide some [security recommendations](
https://www.qemu.org/docs/master/system/security.html?utm_source=chatgpt.com
) to help protect against an attacker within the VM. Largely these
recommendations fall into two categories: 1) expose only the functionality
(in terms of devices) that the guest needs, and 2) run the qemu process inside
of a sandbox so that an escape from the VM into the qemu process on the host
is still difficult to turn into a compromise of anything outside of the
sandbox. Some of the key sandboxing technologies it recommends are
SELinux/App Armor, resource limits and c-groups, Linux namespaces, and Linux
seccomp. While QEMU does provide flags to disable unneeded devices (see for
example the [microvm machine type](
https://www.qemu.org/docs/master/system/i386/microvm.html ), it only natively
provides Linux seccomp sandboxing (via the `--sandbox` option) but defers
further sandboxing to other tools.

The missing sandboxing is usually addressed by using a QEMU-based hypervisor that
provides this security-sandboxing as one of its features, such as
[libvirt](https://www.libvirt.org/). While libvirt is a production-ready,
battle tested software stack, for some cases it may be a much larger (e.g.
maintaining a persistent system-wide configuration database) and more
invasive (e.g. modifying global nftable rules) than what is needed. For
example, an unprivileged user may want to run a batch job (e.g. to run
analysis code from a third party) without creating system-wide network changes
or persistent state. Similarly, one might want to use a custom orchestrator
to manage virtual machine and network state (e.g. with a gitops-based
persistence model) without the duplicated overhead and functionality from
libvirt.

This repository attempts to provide some low-level tools for sandboxing a
virtual machine emulator without trying to provide all of the features of a
full hypervisor stack.  While QEMU is the only emulator currently supported,
other emulators like Firecracker and CloudHypervisor may be supported in the
future (as the sandboxing requirements are nearly identical).  Unlike QEMU
itself, this utility favors security over usability by default, so you will
likely need to turn on features you need but are less likely to be surprised
by an vulnerability in a feature you were not using. It also only aims to
support a subset of QEMU's functionality, so many features like live-migration,
PCIe device hot plugging, and attaching new disks to a running machine may
never be supported.  If you just need a simple and secure sandbox to run a
virtual machine emulator in, however, some of the tools in this repository may
be useful.

## Utilities provided by this repository:

  - `virt-jailer-core`: This is a low-level wrapper for QEMU (or other
    emulators) in the form of a small C utility with minimal dependencies
    that performs the core security work of building the security sandbox and
    lauching QEMU within it. This includes mapping files, unix sockets and TAP
    devices to file descriptors, creating and pivoting into a hardened root
    file system, creating and switching into isolated network, PID, cgroup, and
    IPC namespaces, setting up a minimal set of environment variables,
    optionally applying a provided AppArmor profile, and dropping privileges to
    the minimum set required by qemu. In the git parlance of "plumbing" and
    "porcelain" commands, this is a "plumbing" command designed to to be called
    by other scripts that intentionally favors implementation simplicity over
    attempts to provide a user-friendly interface; for example . It is intended to be run as
    an unprivileged user.
