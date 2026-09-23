# Build

```sh
sh build/build.sh           # host platform (Linux or macOS): build + pack
sh build/verify.sh          # acceptance against .build/out/openfoam-<platform>.zip
sh build/linux/run.sh all   # the same two steps inside Docker, from any checkout
```

Output lands in `.build/out`: `openfoam-<platform>.zip` and `SHA256SUMS`. The
unpacked kit is `.build/kit/openfoam/<platform>/`.

## Files

| File | What it does |
|---|---|
| `config.sh` | every pinned version, URL, checksum and wmake option, with the reasoning |
| `lib.sh` | fetch-and-verify, guarded edits, logging |
| `build.sh` | orchestrator: copy the upstream tree, unpack ThirdParty, `foamConfigurePaths`, `Allwmake`, then `pack.sh` |
| `pack.sh` | assembles the kit: the runtime subset of the build, symlinks dereferenced, host runtime libraries the kit needs staged beside its own, `KIT.env`, `BUILDINFO.txt`, licences, the zip |
| `verify.sh` | unpacks the zip under a path with a space, drops every mode bit (what a node's extractor leaves), applies the controller's executable rule, runs tutorials serial and parallel with `env -i` + `KIT.env`, then audits the file system for anything written outside the kit and the scratch |
| `linux/Dockerfile` | the build image: Ubuntu 22.04 + the toolchain upstream's `doc/Requirements.md` lists |
| `linux/Dockerfile.verify` | a foreign image (Debian 12, no compiler, no libgomp) the kit must start on |
| `linux/run.sh` | local driver: image, named build volume, kit out, verification |

## Knobs

| Variable | Default | Meaning |
|---|---|---|
| `JOBS` | all cores | parallel width of `Allwmake` and the ThirdParty builds |
| `FORCE` | 0 | wipe the build tree first |
| `WORK` | `.build` | build tree location (`/build` inside the Docker driver) |
| `DEPS_DIR` | `$WORK/deps` | where the pinned downloads live; the driver points it at `@Downloads/` |
| `LONG` | 0 | `verify.sh`: also run motorBike (snappyHexMesh, six ranks, minutes) |
| `VERIFY_ROOT` | `$WORK/verify` | `verify.sh`: where to unpack and run |

## The check that matters

A green `Allwmake` on the machine that built the kit proves nothing about a
compute node. Every lesson of the CalculiX kit came from the gap between the
two: a binary that imported the builder's toolchain DLLs, a dylib linked by an
absolute Homebrew path. So `verify.sh` is written to run **elsewhere**: in a
container from a different base image, as an unprivileged user, with the kit
under an arbitrary path and an environment that contains nothing but what
`KIT.env` says. The file-system audit at the end is the containment gate of
the plan: a run may write into the case directory and the scratch, nowhere
else.
