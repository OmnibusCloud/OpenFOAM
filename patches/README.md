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
