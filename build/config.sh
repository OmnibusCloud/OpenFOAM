# Pins and platform configuration. Sourced, never executed.
#
# Every value here is a decision, not a default. PROVENANCE.md records where
# the pins come from; README.md records why the version is what it is.

OPENFOAM_VERSION=v2606
OPENFOAM_API=2606
OPENFOAM_DIR=OpenFOAM-v2606
THIRDPARTY_DIR=ThirdParty-v2606

UPSTREAM_TAG=OpenFOAM-v2606
UPSTREAM_COMMIT=481094fdf34f11ed6d0d603ee59a858a0124236d

OPENFOAM_SRC_URL=https://dl.openfoam.com/source/v2606/OpenFOAM-v2606.tgz
OPENFOAM_SRC_SHA256=2a1310e3ed192cc4c521e1d22dcc176f57bec61160c878dc4348f21d6672294d
OPENFOAM_SRC_SIZE=69422947

THIRDPARTY_URL=https://dl.openfoam.com/source/v2606/ThirdParty-v2606.tar.gz
THIRDPARTY_SHA256=3c7ccd88c5698a9c77a636b01f26e35d7042f15f14c2364cfd2d403030bf3f4a
THIRDPARTY_SIZE=369660343

# ---------------------------------------------------------------------------
# Components
# ---------------------------------------------------------------------------
#
# Built from the ThirdParty pack, whose sources/ directory carries them
# unpacked. The versions are upstream's own for v2606 (ThirdParty SOURCES.md
# and OpenFOAM etc/config.sh/*), restated here so that a change is a visible
# diff rather than a silent consequence of a new pack.

# Open MPI: bundled, because a node has no MPI of its own (OPAL_PREFIX
# relocates it). NOT the pack's copy: the pack ships 4.1.2 (November 2021),
# whose configure cannot read Xcode 15's objdump on Apple Silicon and stops
# with "Could not determine global symbol label prefix" (macOS try 2,
# 2026-09-23). 4.1.8 (February 2025) is the last release of the 4.1 line,
# ABI-compatible with 4.1.2, and is the one pin for every platform - the
# tarball is fetched by checksum into the pack's sources/openmpi/ and
# mirrored in the redistribution release like everything else we compile.
OPENMPI_VERSION=openmpi-4.1.8
OPENMPI_URL=https://download.open-mpi.org/release/open-mpi/v4.1/openmpi-4.1.8.tar.bz2
OPENMPI_SHA256=466f68e3132a1dc02710cc2011fafced8336d98359fa2dae4dddcfd5719f12a9
SCOTCH_VERSION=scotch_6.1.0       # decomposePar's default method - the one the controller writes
FFTW_VERSION=fftw-3.3.10          # function objects (noise, energy spectra)

# Windows: MS-MPI, the node's own, never bundled (plan D-11). The kit is
# built against the MS-MPI SDK's headers and import library, which exist at
# build time only - the kit carries neither, and a node without MS-MPI runs
# the serial Pstream the kit ships beside the MS-MPI one. The runtime
# installer is pinned too: the CI acceptance installs it on a disposable
# Windows runner to prove the parallel path. Both from Microsoft's MS-MPI
# v10.1.3 release page (2025-02).
MSMPI_VERSION=msmpi-10.1.3        # SDK ProductVersion 10.1.12498.18
MSMPI_SDK_URL=https://download.microsoft.com/download/a/5/2/a5207ca5-1203-491a-8fb8-906fd68ae623/msmpisdk.msi
MSMPI_SDK_SHA256=f9174c54feda794586ebd83eea065be4ad38b36f32af6e7dd9158d8fd1c08433
MSMPI_SETUP_URL=https://download.microsoft.com/download/a/5/2/a5207ca5-1203-491a-8fb8-906fd68ae623/msmpisetup.exe
MSMPI_SETUP_SHA256=c305ce3f05d142d519f8dd800d83a4b894fc31bcad30512cefb557feaccbe8b4

# kahip is left out (2026-09-23). The controller writes decomposeParDict for
# every parallel run and names scotch; a case's own choice of method is
# ignored by design (requirements FR-F12), so kahip would never be asked
# for. It costs an OpenMP runtime on every platform (libgomp had to be
# staged into the Linux kit for it alone) and has no OpenMP at all under
# Apple clang, where the fourth macOS build died linking libkahipDecomp
# ("ld: library 'omp' not found").
KAHIP_VERSION=kahip-none

# Left out on purpose. Each gates tooling the OmnibusCloud controller does
# not admit to its allow-list (foamyHexMesh, in-situ output, external
# solvers), each is a component upstream itself cannot cross-build for
# Windows, and CGAL/boost alone are a large share of the build time.
# Re-enable together with the allow-list, never alone. (kahip: above.)
CGAL_VERSION=cgal-none
BOOST_VERSION=boost-none
ADIOS2_VERSION=adios-none
HDF5_VERSION=hdf5-none
METIS_VERSION=metis-none          # not in the pack's sources/ either; scotch and kahip cover decomposition

# ---------------------------------------------------------------------------
# wmake options
# ---------------------------------------------------------------------------
#
# Upstream's defaults, made explicit. DP/Int32 is what every tutorial and the
# overwhelming majority of user cases assume; a case that needs Int64 or SP is
# a different build and is refused by name upstream of the node.

WM_PRECISION_OPTION=DP
WM_LABEL_SIZE=32
WM_COMPILE_OPTION=Opt
WM_MPLIB=OPENMPI

# ---------------------------------------------------------------------------
# Kit contents
# ---------------------------------------------------------------------------
#
# A node never runs a tutorial, so the 109 MB corpus stays in the source
# pack; the kit carries the cases the acceptance (build/verify.sh) and the
# controller's oracle run, plus the one geometry motorBike needs. Tests that
# want the whole corpus take it from the pack.
KIT_TUTORIALS="incompressible/simpleFoam/pitzDaily
incompressible/simpleFoam/motorBike
incompressible/icoFoam/cavity/cavity
incompressible/pimpleFoam/RAS/TJunction
multiphase/interFoam/laminar/damBreak/damBreak
compressible/rhoSimpleFoam/squareBend
basic/potentialFoam/cylinder
resources/geometry/motorBike.obj.gz"

# Symbol tables are stripped from the kit's own binaries and libraries:
# 15-17 % of their size (measured 2026-09-23: libfiniteVolume 49 -> 42 MB,
# platforms/ 562 -> 471 MB), and no effect on the dynamic symbols that
# OpenFOAM's error backtraces resolve. STRIP_KIT=0 keeps them.
STRIP_KIT=1

# ---------------------------------------------------------------------------
# Platform
# ---------------------------------------------------------------------------
#
# Two vocabularies, deliberately: PLATFORM/KIT_FOLDER is the extract-folder
# vocabulary the controller's resolver probes (windows-x64/linux-x64/
# macos-arm64, the same as the CalculiX and Render kits), WM_* is upstream's.

detect_platform() {
    _s=$(uname -s)
    _m=$(uname -m)
    # TARGET=windows-x64 selects the cross-build: upstream's linux64Mingw
    # rules on a Linux host, MinGW-w64 GCC, MS-MPI's SDK for the parallel
    # Pstream. The build tree is named after the HOST (linux64Mingw...), the
    # kit after the target (win64Mingw...): upstream's createMingwRuntime
    # makes the same rename.
    if [ "${TARGET:-}" = "windows-x64" ]; then
        [ "$_s" = "Linux" ] || die "the Windows kit is cross-built on Linux, not on $_s"
        PLATFORM=windows-x64
        WM_ARCH_NAME=linux64
        WM_COMPILER=Mingw
        SO_EXT=dll
        KIT_FOLDER=$PLATFORM
        WM_LABEL_OPTION=Int$WM_LABEL_SIZE
        WM_OPTIONS_EXPECTED=$WM_ARCH_NAME$WM_COMPILER$WM_PRECISION_OPTION$WM_LABEL_OPTION$WM_COMPILE_OPTION
        WM_OPTIONS_RUNTIME=win64$WM_COMPILER$WM_PRECISION_OPTION$WM_LABEL_OPTION$WM_COMPILE_OPTION
        TP_MPI_PLATFORM=$WM_ARCH_NAME$WM_COMPILER
        TP_LIB_PLATFORM=$WM_ARCH_NAME$WM_COMPILER$WM_PRECISION_OPTION$WM_LABEL_OPTION
        CROSS_PREFIX=x86_64-w64-mingw32
        return 0
    fi
    case "$_s" in
        Linux)
            case "$_m" in
                x86_64) PLATFORM=linux-x64; WM_ARCH_NAME=linux64 ;;
                *) die "unsupported Linux architecture: $_m" ;;
            esac
            WM_COMPILER=Gcc
            SO_EXT=so
            ;;
        Darwin)
            case "$_m" in
                arm64) PLATFORM=macos-arm64; WM_ARCH_NAME=darwin64 ;;
                *) die "unsupported macOS architecture: $_m" ;;
            esac
            WM_COMPILER=Clang
            SO_EXT=dylib
            ;;
        *) die "unsupported host: $_s (the Windows kit is a cross-build, see README.md)" ;;
    esac

    KIT_FOLDER=$PLATFORM
    WM_LABEL_OPTION=Int$WM_LABEL_SIZE
    WM_OPTIONS_EXPECTED=$WM_ARCH_NAME$WM_COMPILER$WM_PRECISION_OPTION$WM_LABEL_OPTION$WM_COMPILE_OPTION
    # ThirdParty places MPI under the compiler-level folder and the
    # label/precision-dependent libraries one level deeper.
    TP_MPI_PLATFORM=$WM_ARCH_NAME$WM_COMPILER
    TP_LIB_PLATFORM=$WM_ARCH_NAME$WM_COMPILER$WM_PRECISION_OPTION$WM_LABEL_OPTION
}

# Libraries a compute node is expected to have. Anything else a kit binary
# needs is staged into the kit by pack.sh (libgomp is the known case: the
# build image has it, a minimal node image does not). glibc and libstdc++
# stay the node's own: the kit is built on the oldest glibc it supports
# (README.md), and shipping a libstdc++ without its matching libgcc_s is how
# kits stop loading on the machines that were fine before.
SYSTEM_LIBS_LINUX="libc.so.6 libm.so.6 libpthread.so.0 libdl.so.2 librt.so.1 libutil.so.1 libresolv.so.2 libnsl.so.1 libstdc++.so.6 libgcc_s.so.1 libz.so.1 ld-linux-x86-64.so.2 linux-vdso.so.1"

# The same list for Windows: what every Windows installation has. Everything
# else an executable or DLL of the kit imports must be a DLL in the kit -
# the MinGW runtime (libstdc++-6, libgcc_s_seh-1, libwinpthread-1, zlib1),
# scotch, fftw - and build/windows/pack.sh refuses the kit otherwise. The one
# exception it knows: msmpi.dll, imported by the MS-MPI Pstream alone.
# Compared case-insensitively (a PE import table's spelling varies).
SYSTEM_DLLS_WINDOWS="kernel32.dll msvcrt.dll advapi32.dll user32.dll ws2_32.dll shell32.dll ole32.dll oleaut32.dll shlwapi.dll dbghelp.dll iphlpapi.dll bcrypt.dll ntdll.dll userenv.dll psapi.dll crypt32.dll secur32.dll version.dll wsock32.dll gdi32.dll comdlg32.dll imagehlp.dll rpcrt4.dll"
