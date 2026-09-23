/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2026 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM, distributed under GPL-3.0-or-later.

Description
    Wrapper around rubbish to do with compiling on MacOS with g++15.
    Not yet sure whether this is a long term solution; might also affect
    other files.

\*---------------------------------------------------------------------------*/

#ifndef Foam_dylib_macos_h
#define Foam_dylib_macos_h

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include <mach/port.h>

// macOS 26+ SDK (XNU): mach/port.h defines xnu_static_assert_struct_size
// using _Static_assert (C11), which GCC (non-clang) rejects in C++ mode.
// mach-o/dyld.h transitively includes mach/message.h which uses these macros,
// so redefine them with static_assert (C++) first.
#if defined(__GNUC__) && !defined(__clang__)
#  undef xnu_static_assert_struct_size
#  undef xnu_static_assert_struct_size_kernel_user
#  undef xnu_static_assert_struct_size_kernel_user64_user32
#  if defined(__arm64__)
#    define xnu_static_assert_struct_size(n,s) \
    static_assert(sizeof(n)==(s),"struct size mismatch")
#  else
#    define xnu_static_assert_struct_size(n,s) static_assert(true,"")
#  endif
#  define xnu_static_assert_struct_size_kernel_user(n,ks,us) \
    xnu_static_assert_struct_size(n,us)
#  if defined(__LP64__)
#    define xnu_static_assert_struct_size_kernel_user64_user32(n,ks,u64,u32) \
    xnu_static_assert_struct_size(n,u64)
#  else
#    define xnu_static_assert_struct_size_kernel_user64_user32(n,ks,u64,u32) \
    xnu_static_assert_struct_size(n,u32)
#  endif
#endif

#include <mach-o/dyld.h>

#endif // Foam_dylib_macos_h

// ************************************************************************* //
