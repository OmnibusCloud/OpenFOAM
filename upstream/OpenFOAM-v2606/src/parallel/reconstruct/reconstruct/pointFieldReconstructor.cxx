/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2015 OpenFOAM Foundation
    Copyright (C) 2022-2025 OpenCFD Ltd.
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

#include "pointFieldReconstructor.H"
#include "pointFields.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

int Foam::pointFieldReconstructor::verbose_ = 1;


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::pointFieldReconstructor::pointFieldReconstructor
(
    const pointMesh& mesh,
    const UPtrList<pointMesh>& procMeshes,
    const UPtrList<labelIOList>& pointProcAddressing,
    const UPtrList<labelIOList>& boundaryProcAddressing
)
:
    mesh_(mesh),
    procMeshes_(procMeshes),
    pointProcAddressing_(pointProcAddressing),
    boundaryProcAddressing_(boundaryProcAddressing),
    patchPointAddressing_(procMeshes.size()),
    nReconstructed_(0)
{
    // Inverse-addressing of the patch point labels.
    labelList pointMap(mesh_.size(), -1);

    // Create the pointPatch addressing
    forAll(procMeshes_, proci)
    {
        const pointMesh& procMesh = procMeshes_[proci];

        patchPointAddressing_[proci].resize(procMesh.boundary().size());

        forAll(procMesh.boundary(), patchi)
        {
            if (boundaryProcAddressing_[proci][patchi] >= 0)
            {
                labelList& procPatchAddr = patchPointAddressing_[proci][patchi];
                procPatchAddr.resize(procMesh.boundary()[patchi].size(), -1);

                const labelList& patchPointLabels =
                    mesh_.boundary()[boundaryProcAddressing_[proci][patchi]]
                    .meshPoints();

                // Create the inverse-addressing of the patch point labels.
                forAll(patchPointLabels, pointi)
                {
                    pointMap[patchPointLabels[pointi]] = pointi;
                }

                const labelList& procPatchPoints =
                    procMesh.boundary()[patchi].meshPoints();

                forAll(procPatchPoints, pointi)
                {
                    procPatchAddr[pointi] =
                        pointMap
                        [
                            pointProcAddressing_[proci][procPatchPoints[pointi]]
                        ];
                }

                if (procPatchAddr.contains(-1))
                {
                    FatalErrorInFunction
                        << "Incomplete patch point addressing"
                        << abort(FatalError);
                }
            }
        }
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::label Foam::pointFieldReconstructor::reconstructAllFields
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

        doLocalCode(reconstructPointFields);

        #undef doLocalCode
    }
    while (false);

    return nTotal;
}


// ************************************************************************* //
