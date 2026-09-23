/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2017 Wikki Ltd
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

Class
    Foam::basicSymmetryFaPatchField

Description
    A symmetry patch

Author
    Zeljko Tukovic, FMENA
    Hrvoje Jasak, Wikki Ltd.

SourceFiles
    basicSymmetryFaPatchField.txx

\*---------------------------------------------------------------------------*/

#ifndef Foam_basicSymmetryFaPatchField_H
#define Foam_basicSymmetryFaPatchField_H

#include "transformFaPatchField.H"
#include "symmetryFaPatch.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                  Class basicSymmetryFaPatchField Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class basicSymmetryFaPatchField
:
    public transformFaPatchField<Type>
{
    typedef basicSymmetryFaPatchField<Type> this_bctype;
    typedef transformFaPatchField<Type> parent_bctype;

public:

    // Constructors

        //- Construct from patch and internal field
        basicSymmetryFaPatchField
        (
            const faPatch&,
            const DimensionedField<Type, areaMesh>&
        );

        //- Construct from patch, internal field and dictionary
        //  The "value" entry: NO_READ
        basicSymmetryFaPatchField
        (
            const faPatch&,
            const DimensionedField<Type, areaMesh>&,
            const dictionary&
        );

        //- Construct by mapping onto a new patch
        basicSymmetryFaPatchField
        (
            const this_bctype&,
            const faPatch&,
            const DimensionedField<Type, areaMesh>&,
            const faPatchFieldMapper&
        );

        //- Construct as copy setting internal field reference
        basicSymmetryFaPatchField
        (
            const this_bctype&,
            const DimensionedField<Type, areaMesh>&
        );

        //- No copy without an internal field
        basicSymmetryFaPatchField(const this_bctype&) = delete;

        //- Clone with an internal field reference
        virtual tmp<faPatchField<Type>> clone
        (
            const DimensionedField<Type, areaMesh>& iF
        ) const
        {
            return faPatchField<Type>::Clone(*this, iF);
        }


    // Member Functions

        //- Retrieve patch-normal gradient at boundary
        virtual tmp<Field<Type>> snGrad() const;

        //- Retrieve patch-normal gradient at boundary
        virtual void snGrad(UList<Type>& result) const;

        //- Evaluate the patch field
        virtual void evaluate
        (
            const Pstream::commsTypes commsType = Pstream::commsTypes::buffered
        );

        //- Return face-gradient transform diagonal
        virtual tmp<Field<Type>> snGradTransformDiag() const;


    // Member Operators

        //- Inherit assignment
        using parent_bctype::operator=;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "basicSymmetryFaPatchField.txx"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
