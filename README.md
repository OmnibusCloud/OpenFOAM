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
| MPI | Open MPI 4.1.2 from the ThirdParty pack, bundled | the kit must run parallel on a node that has no MPI; a system MPI is never assumed |
| Decomposition | scotch 6.1.0, kahip 3.15 | `decomposePar` methods a case may name |
| FFTW 3.3.10 | bundled | function objects that need it |
| CGAL / boost, ADIOS2, HDF5, METIS | **not built** | they gate tools the controller does not admit (foamyHexMesh, in-situ output) and are the components upstream itself cannot cross-build for Windows |
| Compiler | system GCC on Linux (Ubuntu 22.04 image: gcc 11, glibc 2.35), Apple clang on macOS | the glibc the kit is built against is the oldest it runs on |

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
| Linux x86-64 | `openfoam/linux-x64/` | building — Phase 0.2 of the plan |
| macOS arm64 | `openfoam/macos-arm64/` | planned — Phase 0.4 |
| Windows x86-64 | `openfoam/windows-x64/` | gated goal — upstream's MinGW cross-build, MS-MPI only when the machine owner has installed it |

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
| `build/pack.sh` | the kit: files, staged runtime libraries, `KIT.env`, `BUILDINFO.txt`, licences, zip |
| `build/verify.sh` | acceptance: tutorials serial and parallel from an arbitrary folder with a scrubbed environment, file-system audit |
| `build/linux/` | Docker image for the build, a foreign image for verification, the local driver |
| `redistribution/` | the source mirror published beside the kits |
| `PROVENANCE.md` | checksums of everything, and how the pack was verified against the tag |
