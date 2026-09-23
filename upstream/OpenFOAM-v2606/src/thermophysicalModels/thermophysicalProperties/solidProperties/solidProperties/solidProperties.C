/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2018-2025 OpenCFD Ltd.
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

#include "solidProperties.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(solidProperties, 0);
    defineRunTimeSelectionTable(solidProperties,);
    defineRunTimeSelectionTable(solidProperties, dictionary);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::solidProperties::solidProperties() noexcept
:
    rho_(0),
    Cp_(0),
    kappa_(0),
    Hf_(0),
    emissivity_(0),
    W_(0),
    nu_(0),
    E_(0)
{}


Foam::solidProperties::solidProperties
(
    scalar rho,
    scalar Cp,
    scalar kappa,
    scalar Hf,
    scalar emissivity,
    scalar W,
    scalar nu,
    scalar E
) noexcept
:
    rho_(rho),
    Cp_(Cp),
    kappa_(kappa),
    Hf_(Hf),
    emissivity_(emissivity),
    W_(W),
    nu_(nu),
    E_(E)
{}


Foam::solidProperties::solidProperties(const dictionary& dict)
:
    rho_(dict.get<scalar>("rho")),
    Cp_(dict.get<scalar>("Cp")),
    kappa_(dict.getCompat<scalar>("kappa", {{"K", 1612}})),
    Hf_(dict.get<scalar>("Hf")),
    emissivity_(dict.get<scalar>("emissivity")),
    W_(dict.get<scalar>("W")),
    nu_(0),
    E_(0)
{
    // Mechanical properties: optional
    dict.readIfPresent("nu", nu_);
    dict.readIfPresent("E", E_);
}


Foam::solidProperties::solidProperties
(
    const dictionary& dict,
    solidProperties::categories category
)
:
    solidProperties()
{
    // Everyone gets density
    rho_ = dict.get<scalar>("rho");

    // Heat of formation, molecular weight
    if (category == categories::REGULAR)
    {
        Hf_ = dict.get<scalar>("Hf");
        W_ = dict.get<scalar>("W");
    }
    else
    {
        // Optional if thermal or mechanical only
        dict.readIfPresent("Hf", Hf_);
        dict.readIfPresent("W", W_);
    }

    // Thermal properties
    if
    (
        (category == categories::REGULAR)
     || (category & categories::THERMAL)
    )
    {
        Cp_ = dict.get<scalar>("Cp");
        kappa_ = dict.getCompat<scalar>("kappa", {{"K", 1612}});

        // Also handle emissivity as mandatory
        emissivity_ = dict.get<scalar>("emissivity");
    }
    else
    {
        // Optional if mechanical only
        dict.readIfPresent("Cp", Cp_);
        dict.readIfPresentCompat("kappa", {{"K", 1612}}, kappa_);
        dict.readIfPresent("emissivity", emissivity_);
    }

    // Mechanical properties
    if
    (
        (category != categories::REGULAR)
     && (category & categories::MECHANICAL)
    )
    {
        nu_ = dict.get<scalar>("nu");
        E_ = dict.get<scalar>("E");
    }
    else
    {
        dict.readIfPresent("nu", nu_);
        dict.readIfPresent("E", E_);
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::solidProperties::readIfPresent(const dictionary& dict)
{
    dict.readIfPresent("rho", rho_);
    dict.readIfPresent("Cp", Cp_);
    dict.readIfPresentCompat("kappa", {{"K", 1612}}, kappa_);
    dict.readIfPresent("Hf", Hf_);
    dict.readIfPresent("emissivity", emissivity_);
    dict.readIfPresent("W", W_);
    dict.readIfPresent("nu", nu_);
    dict.readIfPresent("E", E_);
}


void Foam::solidProperties::writeData(Ostream& os) const
{
    os  << rho_ << token::SPACE
        << Cp_ << token::SPACE
        << kappa_ << token::SPACE
        << Hf_ << token::SPACE
        << emissivity_ << token::SPACE
        << W_ << token::SPACE
        << nu_ << token::SPACE
        << E_;
}


// * * * * * * * * * * * * * * IOStream operators  * * * * * * * * * * * * * //

Foam::Ostream& Foam::operator<<(Ostream& os, const solidProperties& s)
{
    s.writeData(os);
    return os;
}


// ************************************************************************* //
