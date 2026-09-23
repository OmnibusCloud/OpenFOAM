/*---------------------------------------------------------------------------*\
Description
    Compatibility include

    SliceList replaced by StrideList (DEC-2025)

\*---------------------------------------------------------------------------*/

#ifndef FoamCompat_SliceList_H
#define FoamCompat_SliceList_H

#include "StrideList.H"
#include "sliceRange.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

//- Older name for StrideList (2512 and earlier)
template<class T>
using SliceList = StrideList<T>;

} // End namespace Foam


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
