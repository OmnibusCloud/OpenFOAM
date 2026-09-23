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
| Components we compile (unpacked under `sources/`) | `openmpi-4.1.2` (BSD-3-Clause), `scotch_6.1.0` (CeCILL-C), `kahip-3.15` (MIT), `fftw-3.3.10` (GPL-2.0-or-later) |
| Components present but not compiled | `boost_1_74_0`, `CGAL-4.14.3`, `ADIOS2-2.12.1`, `hdf5-2.1.1`, `umpire-2025.03.0`, `ParaView-v6.1.1` |

The choice of what is compiled is a decision recorded in
[`build/config.sh`](build/config.sh): the kit runs whole cases on one node, and
the components left out gate tools (foamyHexMesh, in-situ output, PETSc
solvers) that the OmnibusCloud controller does not admit to its allow-list.

## Kits

Filled in at the first release: per platform, the archive name, SHA-256, size,
the CI run that produced it, and the toolchain the run reported. Until then
there is no kit to cite.

| Kit | Platform | SHA-256 | Size | Built by |
|---|---|---|---|---|
| — | — | — | — | — |

## Windows

OpenCFD publishes a native Windows build of the same version
(`OpenFOAM-v2606-windows-mingw.exe`, 203 MB, MinGW cross-compiled, MS-MPI
optional and installed separately). If OmnibusCloud ever redistributes that
package, or a kit repacked from it, its checksum is recorded here before the
first download link exists — the rule this repository follows for every byte
it hands to other people.
