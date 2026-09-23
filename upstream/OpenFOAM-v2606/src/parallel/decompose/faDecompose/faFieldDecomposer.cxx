/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2017 Wikki Ltd
    Copyright (C) 2021-2026 OpenCFD Ltd.
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

#include "faFieldDecomposer.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::faFieldDecomposer::patchFieldDecomposer::patchFieldDecomposer
(
    const label sizeBeforeMapping,
    const labelUList& addressingSlice,
    const label addressingOffset,
    const bool noEdgeEncoding
)
:
    sizeBeforeMapping_(sizeBeforeMapping),
    directAddressing_(addressingSlice)
{
    forAll(directAddressing_, i)
    {
        if (noEdgeEncoding)
        {
            directAddressing_[i] -= addressingOffset;
        }
        else
        {
            // Subtract one to align addressing.
            directAddressing_[i] -= addressingOffset + 1;
        }
    }
}


Foam::faFieldDecomposer::processorAreaPatchFieldDecomposer::
processorAreaPatchFieldDecomposer
(
    const label nTotalFaces,
    const labelUList& owner,  // == mesh.edgeOwner()
    const labelUList& neigh,  // == mesh.edgeNeighbour()
    const labelUList& addressingSlice,
    const bitSet& flip,
    const bool noEdgeEncoding
)
:
    sizeBeforeMapping_(nTotalFaces),
    directAddressing_(addressingSlice.size())
{
    forAll(directAddressing_, i)
    {
        // Subtract one to align addressing.
        label ai = Foam::mag(addressingSlice[i]) - 1;
        if (noEdgeEncoding)
        {
            // Compat: actually using addressing without a turning index
            ai = addressingSlice[i];
        }

        if (ai < neigh.size())
        {
            // This is a regular edge. it has been an internal edge
            // of the original mesh and now it has become a edge
            // on the parallel boundary

            // With a turning index we can use
            //     'if (addressingSlice[i] >= 0)'
            // to decide the edge is not flipped. But we already have the
            // flip information as a bitSet as well (for the noEdgeEncoding
            // case). So resolve the redundancy in favour of just using the
            // flip map.

            if (!flip.test(i))
            {
                // We are the owner side so use the neighbour value
                directAddressing_[i] = neigh[ai];
            }
            else
            {
                // We are the neighbour side so use the owner value
                directAddressing_[i] = owner[ai];
            }
        }
        else
        {
            // This is a edge that used to be on a cyclic boundary
            // but has now become a parallel patch edge. I cannot
            // do the interpolation properly (I would need to look
            // up the different (edge) list of data), so I will
            // just grab the value from the owner face

            directAddressing_[i] = owner[ai];
        }
    }
}


Foam::faFieldDecomposer::processorEdgePatchFieldDecomposer::
processorEdgePatchFieldDecomposer
(
    label sizeBeforeMapping,
    const labelUList& addressingSlice,
    const bool noEdgeEncoding
)
:
    sizeBeforeMapping_(sizeBeforeMapping),
    addressing_(addressingSlice.size()),
    weights_(addressingSlice.size())
{
    forAll(addressing_, i)
    {
        addressing_[i].resize(1);
        weights_[i].resize(1);

        if (noEdgeEncoding)
        {
            addressing_[i][0] = Foam::mag(addressingSlice[i]);
        }
        else
        {
            addressing_[i][0] = Foam::mag(addressingSlice[i]) - 1;
        }
        weights_[i][0] = 1;
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::faFieldDecomposer::faFieldDecomposer
(
    Foam::zero,
    const faMesh& procMesh,
    const labelUList& edgeAddressing,
    const labelUList& faceAddressing,
    const labelUList& boundaryAddressing,
    const bool disableEdgeEncoding
)
:
    procMesh_(procMesh),
    edgeAddressing_(edgeAddressing),
    faceAddressing_(faceAddressing),
    boundaryAddressing_(boundaryAddressing),
    noEdgeEncoding_(disableEdgeEncoding)
{}


Foam::faFieldDecomposer::faFieldDecomposer
(
    const faMesh& completeMesh,
    const faMesh& procMesh,
    const labelUList& edgeAddressing,
    const labelUList& faceAddressing,
    const labelUList& boundaryAddressing,
    const bool disableEdgeEncoding
)
:
    faFieldDecomposer
    (
        Foam::zero{},
        procMesh,
        edgeAddressing,
        faceAddressing,
        boundaryAddressing,
        disableEdgeEncoding
    )
{
    reset(completeMesh);
}


Foam::faFieldDecomposer::faFieldDecomposer
(
    const label nTotalFaces,
    const UList<labelRange>& boundaryRanges,
    const labelUList& edgeOwner,
    const labelUList& edgeNeigbour,

    const faMesh& procMesh,
    const labelUList& edgeAddressing,
    const labelUList& faceAddressing,
    const labelUList& boundaryAddressing,
    const bool disableEdgeEncoding
)
:
    faFieldDecomposer
    (
        Foam::zero{},
        procMesh,
        edgeAddressing,
        faceAddressing,
        boundaryAddressing,
        disableEdgeEncoding
    )
{
    reset(nTotalFaces, boundaryRanges, edgeOwner, edgeNeigbour);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::faFieldDecomposer::empty() const noexcept
{
    return patchFieldDecomposers_.empty();
}


void Foam::faFieldDecomposer::clear()
{
    patchFieldDecomposers_.clear();
    processorAreaPatchFieldDecomposers_.clear();
    processorEdgePatchFieldDecomposers_.clear();
    edgeSigns_.clear();
}


void Foam::faFieldDecomposer::reset
(
    const label nTotalFaces,
    const UList<labelRange>& boundaryRanges,
    const labelUList& edgeOwner,
    const labelUList& edgeNeigbour
)
{
    const label nMappers = procMesh_.boundary().size();

    patchFieldDecomposers_.resize_null(nMappers);
    processorAreaPatchFieldDecomposers_.resize_null(nMappers);
    processorEdgePatchFieldDecomposers_.resize_null(nMappers);
    edgeSigns_.resize_null(nMappers);

    bitSet flipMap;

    forAll(boundaryAddressing_, patchi)
    {
        const label oldPatchi = boundaryAddressing_[patchi];
        const faPatch& fap = procMesh_.boundary()[patchi];
        const auto patchEdgeAddr = fap.patchSlice(edgeAddressing_);
        const auto patchEdgeOwner = fap.edgeOwner();

        if (oldPatchi >= 0)
        {
            patchFieldDecomposers_.set
            (
                patchi,
                new patchFieldDecomposer
                (
                    boundaryRanges[oldPatchi].size(),
                    patchEdgeAddr,
                    boundaryRanges[oldPatchi].start(),
                    noEdgeEncoding_
                )
            );
        }
        else
        {
            // No oldPatch - is processor patch.
            // The edgeAddressing_ may not have a 'flip' sign, so use the
            // face map to see which side we've got.
            //
            // If edge is known (from the edgeAddressing_), could just use
            // that and/or do an extra sanity check.

            flipMap.clear();
            flipMap.resize(patchEdgeAddr.size());

            forAll(patchEdgeAddr, i)
            {
                const label serialEdgei =
                (
                    noEdgeEncoding_
                  ? patchEdgeAddr[i]
                  : (Foam::mag(patchEdgeAddr[i])-1)
                );

                // The procMesh face mapped to completeMesh
                const label ownFacei = faceAddressing_[patchEdgeOwner[i]];
                flipMap.set(i, (edgeOwner[serialEdgei] != ownFacei));
            }

            processorAreaPatchFieldDecomposers_.set
            (
                patchi,
                new processorAreaPatchFieldDecomposer
                (
                    nTotalFaces,
                    edgeOwner,
                    edgeNeigbour,
                    patchEdgeAddr,
                    flipMap,
                    noEdgeEncoding_
                )
            );

            processorEdgePatchFieldDecomposers_.set
            (
                patchi,
                new processorEdgePatchFieldDecomposer
                (
                    procMesh_.boundary()[patchi].size(),
                    patchEdgeAddr,
                    noEdgeEncoding_
                )
            );

            auto& s = edgeSigns_.emplace_set(patchi, patchEdgeAddr.size());
            forAll(patchEdgeAddr, i)
            {
                s[i] = (flipMap.test(i) ? -1 : 1);
            }
        }
    }
}


void Foam::faFieldDecomposer::reset(const faMesh& completeMesh)
{
    // Create weightings now - needed for proper parallel synchronization
    //// (void)completeMesh.weights();
    // Disabled the above (2022-04-04)
    // Use weights if they already exist, otherwise simply ignore

    const List<labelRange> boundaryRanges
    (
        completeMesh.boundary().patchRanges()
    );

    reset
    (
        completeMesh.nFaces(),  // nTotalFaces
        boundaryRanges,
        completeMesh.edgeOwner(),
        completeMesh.edgeNeighbour()
    );
}


// ************************************************************************* //
