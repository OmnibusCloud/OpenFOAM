/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

Class
    Foam::regionModels::areaSurfaceFilmModels::regionFaIBMList

Description
    Immersed Boundary Method (IBM) for finite area (FA)

SourceFiles
    regionFaIBMList.cxx
    regionFaIBMList.txx

\*---------------------------------------------------------------------------*/

#ifndef Foam_FA_regionFaIBMList_H
#define Foam_FA_regionFaIBMList_H

#include "regionFaIBM.H"
#include "IOdictionary.H"
#include "PtrList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace regionModels
{
namespace areaSurfaceFilmModels
{

/*---------------------------------------------------------------------------*\
                       Class regionFaIBMList Declaration
\*---------------------------------------------------------------------------*/

class regionFaIBMList
:
    public IOdictionary,
    public PtrList<regionFaIBM>
{
    // Private Member Function

        //- Const reference to the list of regionFaIBM
        const UPtrList<regionFaIBM>& ibm_list() const noexcept
        {
            return static_cast<const UPtrList<regionFaIBM>&>(*this);
        }

        //- Non-const reference to the list of regionFaIBM
        PtrList<regionFaIBM>& ibm_list() noexcept
        {
            return static_cast<PtrList<regionFaIBM>&>(*this);
        }


protected:

    // Protected Data

        //- Interpolated velocity
        areaVectorField Ustar_;


public:

    //- Constructor
    regionFaIBMList(const Time& runTime, const faMesh& aMesh);

    //- Destructor
    virtual ~regionFaIBMList() = default;


    // Member Functions

        //- Ustar velocity
        const areaVectorField& Ustar() const noexcept { return Ustar_; }

        //- Ustar velocity
        areaVectorField& Ustar() noexcept { return Ustar_; }


        //- Update mesh
        void updateMesh();

        //- Correct the regionFaIBM model
        bool correct(const areaVectorField& Us);

        //- Apply the zero filter to an area field
        template<class Type>
        void zeroFilter(Type& fld) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace areaSurfaceFilmModels
} // End namespace regionModels
} // End namespace Foam

// ************************************************************************* //

#ifdef NoRepository
    #include "regionFaIBMList.txx"
#endif

// ************************************************************************* //

#endif

// ************************************************************************* //
