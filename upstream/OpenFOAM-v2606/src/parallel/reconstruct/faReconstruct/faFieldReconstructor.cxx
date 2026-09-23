/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2017 Wikki Ltd
    Copyright (C) 2022-2026 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "faFieldReconstructor.H"
#include "areaFields.H"
#include "edgeFields.H"
#include "ListOps.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

int Foam::faFieldReconstructor::verbose_ = 1;


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::faFieldReconstructor::faFieldReconstructor
(
    const faMesh& mesh,
    const UPtrList<faMesh>& procMeshes,
    const UPtrList<labelIOList>& edgeProcAddressing,
    const UPtrList<labelIOList>& faceProcAddressing,
    const UPtrList<labelIOList>& boundaryProcAddressing
)
:
    mesh_(mesh),
    procMeshes_(procMeshes),
    edgeProcAddressing_(edgeProcAddressing),
    faceProcAddressing_(faceProcAddressing),
    boundaryProcAddressing_(boundaryProcAddressing),
    nReconstructed_(0),
    noEdgeEncoding_(false)
{
    // Does edgeProcAddressing use turning index?
    // For 2512 and earlier: without a turning index.
    // Detection as follows:
    //  - none : +ve addresses only and a '0' address (2512 and earlier)
    //  - with : -ve/+ve addresses but no '0' address
    //  .
    forAllReverse(edgeProcAddressing_, proci)
    {
        const auto& addr = edgeProcAddressing_[proci];

        if (auto i = ListOps::find_if(addr, labelRange::le0()); i >= 0)
        {
            if (addr[i] == 0)
            {
                // The value '0' only occurs without edge encoding
                noEdgeEncoding_ = true;
            }
            // else: -ve value - so definitely has edge encoding
            break;
        }
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::label Foam::faFieldReconstructor::reconstructAllFields
(
    const IOobjectList& objects,
    const wordRes& selected
)
{
    label nTotal = 0;

    do
    {
        #undef  doLocalCode
        #define doLocalCode(Method)                                           \
        {                                                                     \
            nTotal += this->Method <scalar> (objects, selected);              \
            nTotal += this->Method <vector> (objects, selected);              \
            nTotal += this->Method <sphericalTensor> (objects, selected);     \
            nTotal += this->Method <symmTensor> (objects, selected);          \
            nTotal += this->Method <tensor> (objects, selected);              \
        }

        doLocalCode(reconstructAreaFields);
        doLocalCode(reconstructEdgeFields);

        #undef doLocalCode
    }
    while (false);

    return nTotal;
}


// ************************************************************************* //
