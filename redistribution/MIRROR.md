# Redistribution mirror

What OmnibusCloud hands to other people, and the source that has to travel
with it. Produced and verified by [`mirror.sh`](mirror.sh); published as the
release `upstream-mirror-v2606` of this repository.

## Why this exists

A kit is a binary distribution of GPL-3 software and of the third-party
components compiled into it. GPL section 6 requires the corresponding source
to be available to whoever receives the binary; pointing at another
organisation's download server is a promise about a host we do not run.
Mirroring the exact packs here, publicly and with no account wall, is what
discharges it. The build pipeline also pulls from this mirror as its fallback
when dl.openfoam.com is unreachable, so a release never depends on a single
host being up that day.

## Inventory

Every file is byte-identical to what upstream published.

| File | What | Licence | SHA-256 | Size |
|---|---|---|---|---|
| `OpenFOAM-v2606.tgz` | the OpenFOAM source pack, also imported verbatim under `upstream/` | GPL-3.0-or-later | `2a1310e3ed192cc4c521e1d22dcc176f57bec61160c878dc4348f21d6672294d` | 69 422 947 |
| `ThirdParty-v2606.tar.gz` | the ThirdParty pack: build scripts (LGPL-3.0) and the unpacked sources of every third-party component, compiled or not | see below | `3c7ccd88c5698a9c77a636b01f26e35d7042f15f14c2364cfd2d403030bf3f4a` | 369 660 343 |
| `openmpi-4.1.8.tar.bz2` | the Open MPI the kits bundle — not the pack's 4.1.2, see `build/config.sh` | BSD-3-Clause | `466f68e3132a1dc02710cc2011fafced8336d98359fa2dae4dddcfd5719f12a9` | (recorded at first mirror run) |

Third-party components the kits compile:

| Component | Source | Licence |
|---|---|---|
| Open MPI 4.1.8 | `openmpi-4.1.8.tar.bz2` above (the pack's `sources/openmpi/openmpi-4.1.2` is not built) | BSD-3-Clause |
| SCOTCH 6.1.0 | `sources/scotch/scotch_6.1.0` | CeCILL-C |
| FFTW 3.3.10 | `sources/fftw/fftw-3.3.10` | GPL-2.0-or-later |

Present in the pack, not compiled (recorded so that a future change is a
visible decision): Open MPI 4.1.2, KaHIP 3.15, boost 1.74.0, CGAL 4.14.3,
ADIOS2 2.12.1, HDF5 2.1.1, umpire 2025.03.0, ParaView 6.1.1.

## Windows

If OmnibusCloud ever redistributes OpenCFD's native Windows package
(`OpenFOAM-v2606-windows-mingw.exe`) or a kit repacked from it, that package
is mirrored here too, with its checksum, before any download link exists.
