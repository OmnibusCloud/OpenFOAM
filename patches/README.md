# Patches

Empty, and that is the point: the OpenFOAM we distribute is upstream's,
unmodified.

If that ever stops being true, every change lands here as a `*.patch` file
applied by `build/build.sh` against the build copy of `upstream/`, never by
editing the imported tree. Two reasons:

- `git log -- upstream/` must keep showing exactly what upstream shipped and
  nothing else, so the tree stays citable as the corresponding source.
- A GPL distributor has to be able to say precisely what it changed. A
  directory of patches answers that; a modified tree does not.

Configuration is not a patch: the build selects options through upstream's
own `bin/tools/foamConfigurePaths` on the build copy, and the resulting
`etc/` travels in the kit, so what the kit runs with is visible in the kit.

## Build-configuration adjustments to the ThirdParty pack

The ThirdParty pack is not in this tree (it is fetched by checksum and
mirrored in the redistribution release), so the build applies its
adjustments to the unpacked copy with guarded edits in `build/build.sh`:
each one asserts the line it expects before editing and the result after,
and stops the build if upstream's file has changed shape. None of them
touches a source file; they are the build flags a platform needs.

| Platform | File | Adjustment | Why |
|---|---|---|---|
| macOS | `etc/makeFiles/scotch/Makefile.inc.Darwin.shlib` | `-DHAVE_SYS_TIME_H -DHAVE_SYS_RESOURCE_H` added to `CFLAGS` | scotch 6.1.0's `common.c` calls `gettimeofday` without including `<sys/time.h>` unless told the header exists; Apple clang 15 makes the implicit declaration an error (found 2026-09-23) |

## Build-configuration adjustments to upstream's `etc-mingw/` overlay (Windows)

Applied by `build/windows/cross.sh` to the build copy, guarded the same way.
`etc-mingw/` is upstream's own configuration for the MinGW cross-build; the
kit carries the adjusted copy, so what it was built with is visible in it.

| File | Adjustment | Why |
|---|---|---|
| `etc-mingw/prefs.sh` | `WM_MPLIB=msmpi-10.0` -> `WM_MPLIB=msmpi-10.1.3` | the version in `WM_MPLIB` is the directory name `etc/config.sh/mpi` resolves under `ThirdParty/platforms/linux64Mingw/`; the pinned SDK is laid out under its own version |
| `etc-mingw/config.sh/FFTW.system` | removed from the overlay | it points at openSUSE's MinGW fftw package (`/usr/x86_64-w64-mingw32/sys-root/mingw`); this build cross-compiles fftw from the ThirdParty pack, which the default `etc/config.sh/FFTW` describes |
| `etc-mingw/prefs.sh` | `export FOAM_EXTRA_CXXFLAGS="${FOAM_EXTRA_CXXFLAGS} -fno-jump-tables"` appended | GCC 13 for x86_64-w64-mingw32 emits the jump table of a switch inside an inline (COMDAT) function into the plain `.rdata` section with a 32-bit section-relative relocation into that function's `.text$` section; upstream partially links OSspecific and Pstream into single objects (`ld -r`), which discards the duplicate inline copies and leaves those jump tables dangling ("relocation truncated to fit: IMAGE_REL_AMD64_REL32 against .text$_ZN4Foam5token5resetEv" when `libOpenFOAM.dll` is linked, 2026-09-24). Without jump tables a switch compiles to compares and branches. In prefs.sh because `etc/bashrc` clears the variable before prefs.sh runs |
