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

OPENMPI_VERSION=openmpi-4.1.2     # bundled: a node has no MPI of its own; OPAL_PREFIX relocates it
SCOTCH_VERSION=scotch_6.1.0       # decomposePar's default method
KAHIP_VERSION=kahip-3.15          # decomposePar method a case may name
FFTW_VERSION=fftw-3.3.10          # function objects (noise, energy spectra)

# Left out on purpose. Each gates tooling the OmnibusCloud controller does
# not admit to its allow-list (foamyHexMesh, in-situ output, external
# solvers), each is a component upstream itself cannot cross-build for
# Windows, and CGAL/boost alone are a large share of the build time.
# Re-enable together with the allow-list, never alone.
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
# Platform
# ---------------------------------------------------------------------------
#
# Two vocabularies, deliberately: PLATFORM/KIT_FOLDER is the extract-folder
# vocabulary the controller's resolver probes (windows-x64/linux-x64/
# macos-arm64, the same as the CalculiX and Render kits), WM_* is upstream's.

detect_platform() {
    _s=$(uname -s)
    _m=$(uname -m)
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
