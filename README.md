# OpenFOAM — OmnibusCloud build and source mirror

This repository is the **corresponding source** and the **build pipeline** for
the OpenFOAM® kits that OmnibusCloud compute nodes run.

OpenFOAM is free software by **OpenCFD Ltd**, licensed under the **GNU General
Public License, version 3 or later**. We do not own it, we did not write it,
and we distribute it unmodified. See [LICENSE](LICENSE), [NOTICE.md](NOTICE.md)
and [PROVENANCE.md](PROVENANCE.md).

OpenFOAM® is a registered trademark of OpenCFD Limited. **This offering is not
approved or endorsed by OpenCFD Limited, producer and distributor of the
OpenFOAM software via www.openfoam.com, and owner of the OPENFOAM® and
OpenCFD® trade marks.**

Upstream lives at <https://www.openfoam.com> and
<https://gitlab.com/openfoam/core/openfoam>.

## Why this repository exists

OmnibusCloud runs OpenFOAM cases as units of a parameter study: one whole case
on one compute node, many variants across an office's machines. The nodes are
heterogeneous (Windows, Linux, macOS) and carry no preinstalled software, so
the solver has to arrive as a **controller asset**: a pinned build that
unpacks into a folder and runs from there.

No such build exists upstream. OpenCFD ships Linux binaries only as
distribution packages linked against the distribution's Open MPI, scotch and
glibc; macOS has no official binary at all; Windows is the one platform with
a native official kit. So we build it, the way we build
[CalculiX](https://github.com/OmnibusCloud/CalculiX): upstream's source
verbatim under [`upstream/`](upstream/), the patches we apply (none) under
[`patches/`](patches/), the whole build under [`build/`](build/). Every kit we
publish can be rebuilt from this tree alone, which is also what discharges
the GPL's obligation to offer the source together with the scripts that
control its compilation.

## What is built

Version **v2606** (the OpenCFD/ESI line, June 2026) — one line, one version,
pinned deliberately: a case runs on a compute node under exactly the build the
user's own screen names, and a case written for another line or version is
refused by name rather than run against a different solver.

| Setting | Value | Why |
|---|---|---|
| Precision / label | `DP`, `Int32` | upstream's defaults; the ones every tutorial and most user cases assume |
| MPI | Linux, macOS: Open MPI 4.1.8, bundled (fetched by checksum; the pack's 4.1.2 does not configure under Xcode 15 on Apple Silicon). Windows: MS-MPI, the node's own - the kit is built against the MS-MPI SDK and ships a serial `libPstream.dll` beside the MS-MPI one | the kit must run parallel on a node that has no MPI; a system MPI is never assumed on Linux and macOS; on Windows MS-MPI's licence allows redistributing only its installer, so it is the machine owner's to install, and the kit still runs serially without it |
| Decomposition | scotch 6.1.0 (and pt-scotch) | the method the controller writes into every `decomposeParDict`; a case's own choice is ignored by design |
| FFTW 3.3.10 | bundled | function objects that need it |
| kahip | **not built** | never asked for (see above); it would cost an OpenMP runtime on every platform and has none under Apple clang |
| CGAL / boost, ADIOS2, HDF5, METIS | **not built** | they gate tools the controller does not admit (foamyHexMesh, in-situ output) and are the components upstream itself cannot cross-build for Windows |
| Compiler | system GCC on Linux (Ubuntu 22.04 image: gcc 11, glibc 2.35), Apple clang on macOS, MinGW-w64 GCC 13 (posix threads, Ubuntu 24.04 image) cross-compiling for Windows | the glibc the kit is built against is the oldest it runs on; Windows is built the way OpenCFD builds its own Windows binaries |

The kit is **relocatable by environment, not by installation**: nothing is
ever sourced or installed on a node. `KIT.env` at the kit's root lists the
exact environment the build establishes (paths relative to `@KIT@`, scratch
locations as `@SCRATCH@`), and the controller sets that environment on the
solver process alone — `HOME` and `TMPDIR` inside the task's scratch, so the
run reads no user configuration and writes nothing outside the case
directory. The verification step ([`build/verify.sh`](build/verify.sh))
audits the file system after a run to prove it.

## Platforms

| Platform | Kit folder | Status |
|---|---|---|
| Linux x86-64 | `openfoam/linux-x64/` | **released** in [`openfoam-v2606-3`](https://github.com/OmnibusCloud/OpenFOAM/releases/tag/openfoam-v2606-3) (152 MB zip, 506 MB unpacked; first in `openfoam-v2606-1`); accepted after download in a foreign image, motorBike included |
| macOS arm64 | `openfoam/macos-arm64/` | **released** in [`openfoam-v2606-3`](https://github.com/OmnibusCloud/OpenFOAM/releases/tag/openfoam-v2606-3) (113 MB zip, 417 MB unpacked; ad-hoc signed; first in `openfoam-v2606-2`); accepted with the build volume detached as a dedicated account, motorBike included |
| Windows x86-64 | `openfoam/windows-x64/` | **released** in [`openfoam-v2606-3`](https://github.com/OmnibusCloud/OpenFOAM/releases/tag/openfoam-v2606-3) (210 MB zip, 683 MB unpacked), cross-built on Linux with MinGW-w64; accepted on a Windows runner with MS-MPI installed - pitzDaily serially as shipped and on four ranks under `mpiexec` after the Pstream swap, motorBike on six ranks, file-system audit clean. MS-MPI is the node's own, never bundled; a node without it runs serially |

**Windows.** The Windows kit is not OpenCFD's official Windows build repacked,
although one exists: its binaries carry a build stamp from December 2021
beside the `v2606` version string, so the exact source they were built from
cannot be named, and a distributor under the GPL has to name it. It is built
here instead, from the same pinned source, with upstream's own
cross-compilation rules; the official build is kept as the reference the
kit's results are compared against. Every DLL sits beside the executables
(where Windows looks first), `libPstream.dll` is the serial one, and the
MS-MPI one travels beside it as `libPstream.dll-msmpi` for the controller to
swap in on a node that has MS-MPI installed.

**macOS signing.** The macOS kit carries ad-hoc signatures only — the ones
Apple's linker applies to every arm64 binary, and an explicit `codesign -s -`
after any change to a binary's library paths. It is not signed with a
Developer ID and not notarized, deliberately: a compute node downloads and
unpacks the kit itself, so no file carries the quarantine attribute and
Gatekeeper never evaluates it, while arm64 only requires that a signature
exist. The CalculiX kit ships the same way. A kit downloaded by hand through
a browser would be quarantined and refused; that is not how kits are
distributed.

Kits are published as release assets `openfoam-<platform>.zip` under tags
`openfoam-v2606-N`, with `SHA256SUMS`. Zip archives carry no symbolic links and
no Unix mode bits by the time a node has unpacked them, so the kit contains
none of the former and `KIT.env` names the directories whose files the
controller marks executable.

## Building

```sh
# Linux, in Docker (also from a Windows checkout):
sh build/linux/run.sh all       # image, build, kit, verification in a foreign image

# Linux or macOS, on the host:
sh build/build.sh               # .build/out/openfoam-<platform>.zip
sh build/verify.sh              # unpack somewhere else, run tutorials, audit the file system

# Windows, cross-built on Linux (or in Docker from a Windows checkout), verified on Windows:
TARGET=windows-x64 sh build/build.sh
sh build/windows/run.sh all     # image, cross-build, then build/verify.ps1 on this Windows machine
```

See [`build/README.md`](build/README.md) for the knobs. CI
([`.github/workflows/build.yml`](.github/workflows/build.yml)) runs the same
scripts; a tag `openfoam-v2606-N` publishes the kits.

**Windows checkouts.** The upstream tree cannot be materialised on NTFS: it
carries 219 symbolic links, file names that differ only by case
(`Instant.H` beside `instant.H`), and three tutorial files with a colon in
their name (`jouleHeatingSource:V`, `jouleHeatingSource:sigma`,
`electricPotential:V`), which Windows would turn into alternate data
streams. Clone with a sparse checkout that leaves `upstream/` out, and allow
git to keep those paths in the index:

```sh
git clone --no-checkout https://github.com/OmnibusCloud/OpenFOAM.git
cd OpenFOAM
git config core.protectNTFS false
git sparse-checkout set --no-cone '/*' '!/upstream/'
git checkout main
```

The build detects the absent tree and unpacks the pinned source pack instead
— the same bytes, verified by checksum — so a Windows machine can still drive
a Linux build in Docker (`sh build/linux/run.sh all`). Never build from
`upstream/` directly on any platform: the build works on a copy.

## Repository layout

| Path | Contents |
|---|---|
| `upstream/OpenFOAM-v2606/` | the official source pack, verbatim, one commit |
| `patches/` | empty by intent |
| `build/config.sh` | every pin and every option, with the reasoning |
| `build/build.sh` | orchestrator: copy, ThirdParty, configure, `Allwmake`, pack |
| `build/pack.sh` | the kit: the runtime subset (no sources, no headers, the acceptance tutorials only), staged runtime libraries, symbol tables stripped, `KIT.env`, `BUILDINFO.txt`, licences, zip |
| `build/verify.sh` | acceptance: tutorials serial and parallel from an arbitrary folder with a scrubbed environment, file-system audit |
| `build/linux/` | Docker image for the build, a foreign image for verification, the local driver |
| `build/windows/` | the cross-build (`cross.sh`, `pack.sh`), its Docker image and local driver; `build/verify.ps1` is the Windows acceptance |
| `redistribution/` | the source mirror published beside the kits |
| `PROVENANCE.md` | checksums of everything, and how the pack was verified against the tag |
