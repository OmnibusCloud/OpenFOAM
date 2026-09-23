# ----------------------------------------------------------------------------
# Preferences for mingw cross-compilation

export FOAM_CONFIG_ETC="etc-mingw"

export WM_COMPILER=Mingw
export WM_MPLIB=msmpi-10.0

unset WM_COMPILE_CONTROL

# No zlib available:
# export WM_COMPILE_CONTROL="~libz"

# No mpi available:
# WM_MPLIB=none

# ----------------------------------------------------------------------------
# [mingw] generally wish to avoid debug symbols from bloating the sizes

case "$FOAM_EXTRA_CXXFLAGS" in
(*-g*)
    FOAM_EXTRA_CXXFLAGS="$(echo "$FOAM_EXTRA_CXXFLAGS" | sed 's/-g[0-9]*//')"
    echo "Removed -g from FOAM_EXTRA_CXXFLAGS ($WM_COMPILER)" 1>&2
    ;;
esac

# ----------------------------------------------------------------------------
