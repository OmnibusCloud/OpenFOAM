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

Class
    Foam::parFaFieldDistributorCache

Description
    Simple container to manage read/write, redistribute finiteArea fields.

SourceFiles
    parFaFieldDistributorCache.cxx

\*---------------------------------------------------------------------------*/

#ifndef Foam_parFaFieldDistributorCache_H
#define Foam_parFaFieldDistributorCache_H

#include "faMesh.H"
#include "faMeshDistributor.H"
#include "areaFieldsFwd.H"
#include "edgeFieldsFwd.H"
#include "PtrList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                 Class parFaFieldDistributorCache Declaration
\*---------------------------------------------------------------------------*/

class parFaFieldDistributorCache
{
    // Private Data

    #undef  declareField
    #define declareField(Type)                                                \
    PtrList<GeometricField<Type, faPatchField, areaMesh>> Type##AreaFields_;  \
    PtrList<GeometricField<Type, faePatchField, edgeMesh>> Type##EdgeFields_;

    declareField(scalar);
    declareField(vector);
    declareField(sphericalTensor);
    declareField(symmTensor);
    declareField(tensor);
    #undef declareField


    // Private Member Functions

        //- Read distributed fields
        template<class BoolListType>
        void readImpl
        (
            const Time& baseRunTime,
            const fileName& proc0CaseName,
            const bool decompose,  // i.e. read from undecomposed case

            const BoolListType& areaMeshOnProc,
            const refPtr<fileOperation>& readHandler,
            const fileName& areaMeshInstance,
            faMesh& mesh
        );


public:

    // Constructors

        //- No copy construct
        parFaFieldDistributorCache(const parFaFieldDistributorCache&) = delete;

        //- No copy assignment
        void operator=(const parFaFieldDistributorCache&) = delete;

        //- Default construct
        parFaFieldDistributorCache() = default;


    // Member Functions

        //- Nothing in the cache
        bool empty() const;

        //- Report names of cached fields
        void info(Ostream& os) const;

        //- Read distributed fields
        void read
        (
            const Time& baseRunTime,
            const fileName& proc0CaseName,
            //! Read from undecomposed case? (ignores \p readHandler)
            const bool decompose,
            //! Existence of the processor-local mesh (bitset)
            const bitSet& areaMeshOnProc,
            const refPtr<fileOperation>& readHandler,
            const fileName& areaMeshInstance,
            faMesh& mesh
        );

        //- Read distributed fields
        void read
        (
            const Time& baseRunTime,
            const fileName& proc0CaseName,
            //! Read from undecomposed case? (ignores \p readHandler)
            const bool decompose,
            //! Existence of the processor-local mesh (bool list)
            const boolUList& areaMeshOnProc,
            const refPtr<fileOperation>& readHandler,
            const fileName& areaMeshInstance,
            faMesh& mesh
        );

        //- Redistribute and write all cached area, edge fields
        void redistributeAndWrite(const faMeshDistributor& distributor);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
