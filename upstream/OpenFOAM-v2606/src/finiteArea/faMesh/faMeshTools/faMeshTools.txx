/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

#include "edgeFields.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type, template<class> class PatchField, class GeoMesh>
void Foam::faMeshTools::flatBoundaryField_impl
(
    UList<Type>& result,
    const GeometricField<Type, PatchField, GeoMesh>& fld,
    const bool primitiveOrdering
)
{
    const auto& mesh = fld.mesh();

    #ifdef FULLDEBUG
    if (FOAM_UNLIKELY(result.size() != mesh.nBoundaryEdges()))
    {
        FatalErrorInFunction
            << "mesh nBoundaryEdges = " << mesh.nBoundaryEdges()
            << " but result size = " << result.size()
            << abort(FatalError);
    }
    #endif

    // Some patches (eg empty) may not contribute values. Init to zero
    result = Foam::zero{};

    // Destination index. Starts at 0 since this is a boundary slice.
    label start = 0;

    // Offset (for primitiveOrdering)
    const label nInternal = mesh.nInternalEdges();

    // Boundary fields
    const auto& bfields = fld.boundaryField();

    forAll(bfields, patchi)
    {
        const auto& edgeLabels = mesh.boundary()[patchi].edgeLabels();
        const auto count = edgeLabels.size();
        const auto& pfld = fld.boundaryField()[patchi];

        // Only assign when field size matches underlying patch size
        // ie, skip 'empty' patches etc

        if (count == pfld.size())
        {
            if (primitiveOrdering)
            {
                // In primitive patch order
                forAll(edgeLabels, i)
                {
                    label edgeLocation = (edgeLabels[i] - nInternal);
                    result[edgeLocation] = pfld[i];
                }
            }
            else
            {
                // In sub-list (slice) order
                result.slice(start, count) = pfld;
            }
        }

        start += count;
    }
}


template<class Type>
void Foam::faMeshTools::flatBoundaryField
(
    UList<Type>& result,
    const GeometricField<Type, faPatchField, areaMesh>& fld,
    const bool primitiveOrdering
)
{
    flatBoundaryField_impl(result, fld, primitiveOrdering);
}


template<class Type>
void Foam::faMeshTools::flatBoundaryField
(
    UList<Type>& result,
    const GeometricField<Type, faePatchField, edgeMesh>& fld,
    const bool primitiveOrdering
)
{
    flatBoundaryField_impl(result, fld, primitiveOrdering);
}


template<class Type>
Foam::tmp<Foam::Field<Type>> Foam::faMeshTools::flatBoundaryField
(
    const GeometricField<Type, faPatchField, areaMesh>& fld,
    const bool primitiveOrdering
)
{
    auto tresult = tmp<Field<Type>>::New(fld.mesh().nBoundaryEdges());
    flatBoundaryField_impl(tresult.ref(), fld, primitiveOrdering);
    return tresult;
}


template<class Type>
Foam::tmp<Foam::Field<Type>> Foam::faMeshTools::flatBoundaryField
(
    const GeometricField<Type, faePatchField, edgeMesh>& fld,
    const bool primitiveOrdering
)
{
    auto tresult = tmp<Field<Type>>::New(fld.mesh().nBoundaryEdges());
    flatBoundaryField_impl(tresult.ref(), fld, primitiveOrdering);
    return tresult;
}


template<class Type>
void Foam::faMeshTools::flattenEdgeField
(
    UList<Type>& result,
    const GeometricField<Type, faePatchField, edgeMesh>& fld,
    const bool primitiveOrdering
)
{
    const auto& mesh = fld.mesh();

    #ifdef FULLDEBUG
    if (FOAM_UNLIKELY(result.size() != mesh.nEdges()))
    {
        FatalErrorInFunction
            << "mesh nEdges = " << mesh.nEdges()
            << " but result size = " << result.size()
            << abort(FatalError);
    }
    #endif

    // Internal field
    result.slice(0, fld.size()) = fld.primitiveField();

    // Boundary fields
    auto bfields = result.slice(mesh.nInternalEdges(), mesh.nBoundaryEdges());
    flatBoundaryField(bfields, fld, primitiveOrdering);
}


template<class Type>
Foam::tmp<Foam::Field<Type>> Foam::faMeshTools::flattenEdgeField
(
    const GeometricField<Type, faePatchField, edgeMesh>& fld,
    const bool primitiveOrdering
)
{
    auto tresult = tmp<Field<Type>>::New(fld.mesh().nEdges());
    flattenEdgeField(tresult.ref(), fld, primitiveOrdering);
    return tresult;
}


// ************************************************************************* //
