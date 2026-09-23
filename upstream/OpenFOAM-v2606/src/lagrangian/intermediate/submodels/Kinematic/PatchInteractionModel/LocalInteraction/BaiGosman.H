/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2026 Keysight Technologies
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
    Foam::localInteractionModels::BaiGosman

Description
    Internal BaiGosman thermo parcel-patch interaction used by
    Foam::LocalInteraction.

    Requires a parcel type exposing T() and rho() (thermo / reacting
    clouds) and an SLGThermo with at least one liquid component
    registered on the cloud's database. Splash parcels are added to the
    owner cloud immediately via Cloud::addParticle.

\*---------------------------------------------------------------------------*/

#ifndef Foam_BaiGosman_H
#define Foam_BaiGosman_H

#include "patchInteractionData.H"
#include "polyPatch.H"
#include <type_traits>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class Random;
class SLGThermo;
template<class CloudType> class LocalInteraction;

namespace localInteractionModels
{

template<class ParcelType, class = void>
struct parcelHasThermo
:
    std::false_type
{};


template<class ParcelType>
struct parcelHasThermo
<
    ParcelType,
    std::void_t
    <
        decltype(std::declval<ParcelType&>().T()),
        decltype(std::declval<ParcelType&>().rho())
    >
>
:
    std::true_type
{};


/*---------------------------------------------------------------------------*\
                         Class BaiGosman Declaration
\*---------------------------------------------------------------------------*/

template<class CloudType>
class BaiGosman
{
    // Private Typedefs

        //- Convenience typedef for this cloud's parcel type
        typedef typename CloudType::parcelType parcelType;


    // Private Data

        //- Owning localInteraction model
        LocalInteraction<CloudType>& localInteraction_;

        //- Reference to cloud random number generator
        Random& rndGen_;

        //- Thermo package for temperature-dependent liquid properties
        const SLGThermo* thermoPtr_;


    // Private Constructors

        //- Construct from owning localInteraction model
        explicit BaiGosman(LocalInteraction<CloudType>& localInteraction);

        //- Copy construct with a new owning localInteraction model
        BaiGosman
        (
            const BaiGosman<CloudType>& bg,
            LocalInteraction<CloudType>& localInteraction
        );


    // Private Member Functions

        //- Initialise support for thermo parcels
        void initialise(std::true_type);

        //- Reject support for non-thermo parcels
        void initialise(std::false_type);

        //- Return thermo package
        const SLGThermo& thermo() const;

        //- Return a vector tangential to input vector 'v'
        vector tangentVector(const vector& v) const;

        //- Return splash direction
        vector splashDirection
        (
            const vector& tanVec1,
            const vector& tanVec2,
            const vector& nf
        ) const;

        //- Stick/deposit interaction helper
        void absorbInteraction
        (
            typename CloudType::parcelType& p,
            const polyPatch& pp,
            const label patchi,
            const label idx,
            const scalar mass,
            bool& keepParticle,
            const bool retainParticle = true,
            const bool incrementStickCount = true
        );

        //- Rebound interaction helper with restitution coefficient 'e'
        void bounceInteraction
        (
            const scalar e,
            const vector& Un,
            vector& U
        ) const;

        //- Splash interaction helper
        void splashInteraction
        (
            typename CloudType::parcelType& p,
            const polyPatch& pp,
            const label facei,
            const label patchi,
            const label idx,
            const scalar mRatio,
            const scalar We,
            const scalar Wec,
            const scalar sigma,
            const patchInteractionData& coeffs,
            bool& keepParticle
        );

        //- Dry-wall BaiGosman splash/adhesion model
        void drySplashInteraction
        (
            const scalar sigma,
            const scalar mu,
            const polyPatch& pp,
            const label facei,
            typename CloudType::parcelType& p,
            const vector& nf,
            const patchInteractionData& coeffs,
            const label patchi,
            const label idx,
            bool& keepParticle
        );

        //- Wet-wall BaiGosman splash/adhesion/bounce model
        void wetSplashInteraction
        (
            const scalar sigma,
            const scalar mu,
            const polyPatch& pp,
            const label facei,
            typename CloudType::parcelType& p,
            const vector& nf,
            const patchInteractionData& coeffs,
            const label patchi,
            const label idx,
            bool& keepParticle
        );

        //- Apply BaiGosman model for thermo parcels
        bool correct
        (
            typename CloudType::parcelType& p,
            const polyPatch& pp,
            const label patchi,
            const label idx,
            bool& keepParticle,
            std::true_type
        );

        //- Reject BaiGosman model for non-thermo parcels
        bool correct
        (
            typename CloudType::parcelType& p,
            const polyPatch& pp,
            const label patchi,
            const label idx,
            bool& keepParticle,
            std::false_type
        );


    // Friends

        friend class Foam::LocalInteraction<CloudType>;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace localInteractionModels
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
