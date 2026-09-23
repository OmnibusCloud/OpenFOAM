/*---------------------------------------------------------------------------*\
Description
    Compatibility include

    sliceRange replaced by StrideRange (DEC-2025)

\*---------------------------------------------------------------------------*/

#ifndef FoamCompat_sliceRange_H
#define FoamCompat_sliceRange_H

#include "StrideRange.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

//- Older name for stride range (2512 and earlier)
typedef StrideRange<label> sliceRange;

} // End namespace Foam


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
