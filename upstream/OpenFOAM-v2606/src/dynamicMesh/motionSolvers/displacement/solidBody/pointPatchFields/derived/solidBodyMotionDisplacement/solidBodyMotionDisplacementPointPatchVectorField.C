/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2016 OpenFOAM Foundation
    Copyright (C) 2020 OpenCFD Ltd.
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

#include "solidBodyMotionDisplacementPointPatchVectorField.H"
#include "transformField.H"
#include "addToRunTimeSelectionTable.H"
#include "pointPatchFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * Constructors * * * * * * * * * * * * * * * //

solidBodyMotionDisplacementPointPatchVectorField::
solidBodyMotionDisplacementPointPatchVectorField
(
    const pointPatch& p,
    const DimensionedField<vector, pointMesh>& iF
)
:
    parent_bctype(p, iF),
    SBMFPtr_(nullptr),
    localPoints0Ptr_(nullptr)
{}


solidBodyMotionDisplacementPointPatchVectorField::
solidBodyMotionDisplacementPointPatchVectorField
(
    const pointPatch& p,
    const DimensionedField<vector, pointMesh>& iF,
    const dictionary& dict
)
:
    parent_bctype(p, iF, dict, IOobjectOption::NO_READ),
    SBMFPtr_(solidBodyMotionFunction::New(dict, this->db().time())),
    localPoints0Ptr_(nullptr)
{
    if (!dict.found("value"))
    {
        const auto& pts0 = localPoints0();

        // Determine current local points and offset
        fixedValuePointPatchVectorField::operator==
        (
            transformPoints(SBMFPtr_().transformation(), pts0) - pts0
        );
    }
}


solidBodyMotionDisplacementPointPatchVectorField::
solidBodyMotionDisplacementPointPatchVectorField
(
    const this_bctype& ptf,
    const pointPatch& p,
    const DimensionedField<vector, pointMesh>& iF,
    const pointPatchFieldMapper& mapper
)
:
    parent_bctype(ptf, p, iF, mapper),
    SBMFPtr_(ptf.SBMFPtr_().clone()),
    localPoints0Ptr_(nullptr)
{
    // For safety re-evaluate

    const auto& pts0 = localPoints0();

    fixedValuePointPatchVectorField::operator==
    (
        transformPoints(SBMFPtr_().transformation(), pts0) - pts0
    );
}


solidBodyMotionDisplacementPointPatchVectorField::
solidBodyMotionDisplacementPointPatchVectorField
(
    const this_bctype& ptf,
    const DimensionedField<vector, pointMesh>& iF
)
:
    parent_bctype(ptf, iF),
    SBMFPtr_(ptf.SBMFPtr_().clone()),
    localPoints0Ptr_(nullptr)
{
    // For safety re-evaluate

    const auto& pts0 = localPoints0();

    fixedValuePointPatchVectorField::operator==
    (
        transformPoints(SBMFPtr_().transformation(), pts0) - pts0
    );
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

const pointField&
solidBodyMotionDisplacementPointPatchVectorField::localPoints0() const
{
    if (!localPoints0Ptr_)
    {
        pointIOField points0
        (
            IOobject
            (
                "points",
                this->db().time().constant(),
                polyMesh::meshSubDir,
                this->db(),
                IOobject::MUST_READ,
                IOobject::NO_WRITE,
                IOobject::NO_REGISTER
            )
        );

        localPoints0Ptr_.reset(new pointField(points0, patch().meshPoints()));
    }

    return *localPoints0Ptr_;
}


void solidBodyMotionDisplacementPointPatchVectorField::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }

    const auto& pts0 = localPoints0();

    // Determine current local points and offset
    fixedValuePointPatchVectorField::operator==
    (
        transformPoints(SBMFPtr_().transformation(), pts0) - pts0
    );

    this->parent_bctype::updateCoeffs();
}


void solidBodyMotionDisplacementPointPatchVectorField::
write(Ostream& os) const
{
    // Note: write value
    this->parent_bctype::write(os);

    os.writeEntry(solidBodyMotionFunction::typeName, SBMFPtr_->type());

    os  << indent << word(SBMFPtr_->type() + "Coeffs");
    SBMFPtr_->writeData(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

makePointPatchTypeField
(
    pointPatchVectorField,
    solidBodyMotionDisplacementPointPatchVectorField
);


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
