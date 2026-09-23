# Provenance

Everything this repository builds from, and everything OmnibusCloud
distributes from it, is listed here with the checksum it was verified against.
Recorded 2026-09-23.

## Upstream OpenFOAM source

The tree under [`upstream/OpenFOAM-v2606`](upstream/OpenFOAM-v2606) is the
verbatim content of the official source pack. It is imported in its own
commit, with no edits, so that `git log -- upstream/` shows exactly what
upstream shipped and nothing else.

| Item | Value |
|---|---|
| URL | <https://dl.openfoam.com/source/v2606/OpenFOAM-v2606.tgz> |
| SHA-256 | `2a1310e3ed192cc4c521e1d22dcc176f57bec61160c878dc4348f21d6672294d` |
| Size | 69 422 947 bytes |
| Upstream release | v2606, announced 2026-06-26 |
| Git tag | `OpenFOAM-v2606` = `481094fdf34f11ed6d0d603ee59a858a0124236d` at <https://gitlab.com/openfoam/core/openfoam> (committed 2026-06-19) |
| `META-INFO/build-info` | `build=_481094fdf3-20260618` |
| `META-INFO/api-info` | `api=2606`, `patch=0` |
| Licence | GNU GPL version 3 or later (`upstream/OpenFOAM-v2606/LICENSE.md`; the text is [LICENSE](LICENSE)) |

**Pack versus tag.** `diff -rq --no-dereference` between the unpacked source
pack and a clone of the tag shows no content difference in any file both
carry. The pack additionally carries `META-INFO/{build-info,api-info,
manifest.txt,manifest-modules.txt,manifest-plugins.txt}` and the checked-out
git submodules (`modules/OpenQBMM`, `modules/adios`, `modules/external-solver`,
`modules/visualization`, `plugins/cfmesh`, `plugins/avalanche`,
`plugins/turbulence-community`, `plugins/bindings/*`), which a plain clone
leaves empty. The kits are built from the core tree only (`Allwmake`, not
`Allwmake-modules` / `Allwmake-plugins`), so the submodule content is imported
for completeness of the pack, not compiled.

Modifications: **none.** Should that ever change, every patch goes in
[`patches/`](patches/) and is applied by the build script, never by hand.

## ThirdParty pack

Not imported into this tree — it is 370 MB of other projects' sources, already
unpacked under `sources/`. The build fetches it by URL and refuses a checksum
mismatch; the redistribution mirror publishes the same bytes so that the
corresponding source of every compiled component travels with the kits.

| Item | Value |
|---|---|
| URL | <https://dl.openfoam.com/source/v2606/ThirdParty-v2606.tar.gz> |
| SHA-256 | `3c7ccd88c5698a9c77a636b01f26e35d7042f15f14c2364cfd2d403030bf3f4a` |
| Size | 369 660 343 bytes |
| Scripts licence | LGPL-3.0 (`ThirdParty-v2606/COPYING`) |
| Components we compile (unpacked under `sources/`) | `scotch_6.1.0` (CeCILL-C), `fftw-3.3.10` (GPL-2.0-or-later) |
| Components present but not compiled | `openmpi-4.1.2` (replaced by 4.1.8, below), `kahip-3.15` (never asked for: the controller writes scotch into every `decomposeParDict`; it needs an OpenMP runtime Apple clang lacks), `boost_1_74_0`, `CGAL-4.14.3`, `ADIOS2-2.12.1`, `hdf5-2.1.1`, `umpire-2025.03.0`, `ParaView-v6.1.1` |

## Open MPI

The kits bundle Open MPI **4.1.8**, not the pack's 4.1.2: the 2021 configure
script cannot read the object files Xcode 15's `objdump` produces on Apple
Silicon and stops with "Could not determine global symbol label prefix"
(second macOS build, 2026-09-23). 4.1.8 is the last release of the 4.1 line
and ABI-compatible; one version travels on every platform.

| Item | Value |
|---|---|
| URL | <https://download.open-mpi.org/release/open-mpi/v4.1/openmpi-4.1.8.tar.bz2> |
| SHA-256 | `466f68e3132a1dc02710cc2011fafced8336d98359fa2dae4dddcfd5719f12a9` |
| Released | 2025-02-04 |
| Licence | BSD-3-Clause |

The choice of what is compiled is a decision recorded in
[`build/config.sh`](build/config.sh): the kit runs whole cases on one node, and
the components left out gate tools (foamyHexMesh, in-situ output, PETSc
solvers) that the OmnibusCloud controller does not admit to its allow-list.

## Kits

Per release and platform: the archive, its SHA-256 and size, the commit and CI
run that produced it, and the toolchain the run reported. Every kit listed was
downloaded from its release after publication, checked against `SHA256SUMS`,
and accepted again by `build/verify.sh` (the long run, motorBike included) in
a foreign image.

| Release | Kit | SHA-256 | Size | Built from | Built by |
|---|---|---|---|---|---|
| [`openfoam-v2606-2`](https://github.com/OmnibusCloud/OpenFOAM/releases/tag/openfoam-v2606-2) (2026-09-23) | `openfoam-linux-x64.zip` | `fd0b444ff8b3f63e594a45ae7514a699899c4176bdbdacaf4a52a49be51c7c55` | 158 775 859 | `6eada66` | CI run 35878648232, `ubuntu-22.04`, gcc 11.4.0 |
| [`openfoam-v2606-2`](https://github.com/OmnibusCloud/OpenFOAM/releases/tag/openfoam-v2606-2) (2026-09-23) | `openfoam-macos-arm64.zip` | `f1d86fd29ea76e640b3ef344a690505a5f24589c162c04b2eaabe28a917da71c` | 114 973 131 | `6eada66` | CI run 35878648232, `macos-14`, Apple clang (Xcode 15.4); ad-hoc signed |
| [`openfoam-v2606-1`](https://github.com/OmnibusCloud/OpenFOAM/releases/tag/openfoam-v2606-1) (2026-09-23) | `openfoam-linux-x64.zip` | `601ead350823cdfe9175a17338870c02733683a4eb731c07f1535a100fbfcd2f` | 158 860 111 | `99d2874` | CI run 35852177714, `ubuntu-22.04`, gcc 11.4.0 |

All three: OpenFOAM v2606, Open MPI 4.1.8, scotch 6.1.0, fftw 3.3.10,
DP/Int32/Opt, no kahip.

Acceptance of `openfoam-v2606-2`:

- **Linux, downloaded** (debian:12, unprivileged, `env -i` + `KIT.env`):
  pitzDaily serial 281 iterations and on four ranks 289, damBreak, motorBike
  through snappyHexMesh and simpleFoam on six ranks to Time = 500 in 235 s,
  file-system audit clean.
- **macOS, in the release run** (the kit staged off the build volume, the
  volume detached, a dedicated unprivileged account): pitzDaily serial and on
  four ranks, damBreak, motorBike on six ranks to Time = 500 in 770 s on the
  3-core runner, file-system audit clean. **Downloaded** and inspected
  (`llvm-objdump --macho`): 1 324 files, no symbolic links, no paths that
  differ by case alone, no link-time files, none of the 643 Mach-O files
  loading from outside the kit or the OS, no absolute rpath.

`openfoam-v2606-1` (Linux only) passed the same Linux acceptance after
download (motorBike 253 s); it still carries `lib/libOSspecific.o`, a
link-time object later packs leave out.

## Windows

OpenCFD publishes a native Windows build of the same version. Recorded
2026-09-23 from the downloaded installer, before any decision to redistribute
it or a kit repacked from it — the rule this repository follows for every
byte it hands to other people:

| Item | Value |
|---|---|
| URL | <https://dl.openfoam.com/source/v2606/OpenFOAM-v2606-windows-mingw.exe> |
| SHA-256 | `020321d58b42e1d2ad0b616cdde17f92503608edd902388fb8c73e94e9cb1e25` |
| Size | 203 083 004 bytes |
| Form | NSIS installer: `msys64.7z` (185 MB, an MSYS2 tree with the OpenFOAM installation at `msys64/home/ofuser/OpenFOAM/OpenFOAM-v2606`, 725 MB unpacked: `platforms/win64MingwDPInt32Opt/bin` 622 MB with 270 `.exe` and 139 `.dll`, `tutorials` 99 MB), `thirdParty/msmpisetup.exe` (MS-MPI 10, run by the installer in admin mode or by the user), `setEnvVariables-v2606.bat` (the environment: `HOME`, `WM_PROJECT_DIR`, `PATH` to `platforms/…/bin`, `FOAM_SIGFPE`), documents, licence |
| Runtime | every DLL beside the executables: `libstdc++-6`, `libgcc_s_seh-1`, `libwinpthread-1`, `libfftw3-3`, `libscotch`, the decomposition libraries; **`libPstream.dll` is the MS-MPI variant** (`libPstream.dll-msmpi`, identical checksum), `libPstream.dll-dummy` is the serial stand-in to copy over it on a machine without MS-MPI |
| README | "cross-compiled in OpenSUSE environment using mingw cross-compiler … the thirdparty remain same as of 2512 release … does not support the compilation of OpenFOAM or dynamic code" |

Nothing from this package is redistributed yet; the entry exists so that a
Phase 0.5 interim Windows kit, if built, cites its origin by checksum.
