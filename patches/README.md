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
