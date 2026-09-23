# Written offer of corresponding source

This kit contains programs and libraries compiled from free software:

| Component | Version | Licence |
|---|---|---|
| OpenFOAM® (OpenCFD Ltd) | v2606 | GNU GPL v3 or later |
| Open MPI | 4.1.8 | BSD-3-Clause |
| SCOTCH | 6.1.0 | CeCILL-C |
| FFTW | 3.3.10 | GNU GPL v2 or later |

OmnibusCloud built these binaries and distributes them under those licences.
The **complete corresponding source**, together with the scripts that control
its compilation, is publicly available with no account or fee:

- <https://github.com/OmnibusCloud/OpenFOAM> — the upstream OpenFOAM source
  verbatim (`upstream/`), the build scripts (`build/`), and the release
  matching this kit (the tag named in `BUILDINFO.txt`, "built from").
- The release `upstream-mirror-v2606` of that repository — the upstream
  source pack and the ThirdParty pack the third-party components were
  compiled from, byte-identical to what their authors published, with
  checksums.

Should that repository ever be unreachable, this written offer stands: send a
request to OmnibusCloud (see <https://omnibuscloud.com>) and the source will be
provided on a durable medium for no more than the cost of performing the
distribution. The offer is valid for at least three years from the date in
`BUILDINFO.txt`, and for as long as OmnibusCloud distributes this kit.

OpenFOAM® is a registered trademark of OpenCFD Limited. This offering is not
approved or endorsed by OpenCFD Limited, producer and distributor of the
OpenFOAM software via www.openfoam.com, and owner of the OPENFOAM® and
OpenCFD® trade marks.
