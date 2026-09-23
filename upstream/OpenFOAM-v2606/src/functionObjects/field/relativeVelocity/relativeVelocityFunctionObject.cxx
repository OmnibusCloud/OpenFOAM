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

\*---------------------------------------------------------------------------*/

#include "relativeVelocityFunctionObject.H"
#include "fvMesh.H"
#include "volFields.H"
#include "IOMRFZoneList.H"
#include "rotatingMotion.H"
#include "solidBodyMotionSolver.H"
#include "dictionary.H"
#include "wordRes.H"
#include "mathematicalConstants.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
    defineTypeNameAndDebug(relativeVelocityFunctionObject, 0);
    addToRunTimeSelectionTable
    (
        functionObject,
        relativeVelocityFunctionObject,
        dictionary
    );
}
}


const Foam::Enum
<
    Foam::functionObjects::relativeVelocityFunctionObject::backgroundMode
>
Foam::functionObjects::relativeVelocityFunctionObject::backgroundModeNames_
({
    { backgroundMode::ZERO, "zero" },
    { backgroundMode::SPECIFIED, "specified" },
    { backgroundMode::COPY, "copy" },
});


const Foam::Enum
<
    Foam::functionObjects::relativeVelocityFunctionObject::rotationMode
>
Foam::functionObjects::relativeVelocityFunctionObject::rotationModeNames_
({
    { rotationMode::SPECIFIED, "specified" },
    { rotationMode::MRF, "MRF" },
    { rotationMode::SOLID_BODY_ROTATION, "solidBodyRotation" },
});


// * * * * * * * * * * * * * * * Local Functions * * * * * * * * * * * * * * //

namespace
{

// Set omega specification from dictionary entries:
//
// - "axis"
// - "omega" | "rpm" | "n"
//
Foam::vector getSpecifiedOmega(const Foam::dictionary& coeffs)
{
    using namespace Foam;

    vector axis = coeffs.get<vector>("axis");
    axis.normalise();

    scalar omega(0);

    // Specified 'omega'
    bool done = coeffs.readIfPresent("omega", omega);

    if (!done && coeffs.readIfPresent("rpm", omega))
    {
        // Alternative: specify 'rpm'
        // Convert: rpm -> rad/s
        omega *= constant::mathematical::twoPi / 60;
        done = true;
    }
    if (!done && coeffs.readIfPresent("n", omega))
    {
        // Alternative: specify 'n' (rev/s)
        // Convert: rev/s -> rad/s
        omega *= constant::mathematical::twoPi;
        done = true;
    }

    if (!done)
    {
        // Not yet found?
        // Make it mandatory (for error message)
        coeffs.readEntry("omega", omega);
    }

    // Return as angular velocity (vector)
    return (omega * axis);
}

} // End anonymous namespace


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::functionObjects::relativeVelocityFunctionObject::init
(
    const dictionary& dict,
    const bool verbose
)
{
    // Entries for specified
    {
        origin_.clear();
        omega_.clear();
        zoneName_.clear();
    }

    // Entries for MRF zones
    {
        MRFlist_ = nullptr;
    }

    // Entries for solid body rotationMotion
    {
        sbmSolver_ = nullptr;
        rotatingPtr_ = nullptr;
    }

    switch (rotationMode_)
    {
        case rotationMode::SPECIFIED :
        {
            const auto& spec = dict.subDict("specified", keyType::LITERAL);

            label count = 1;

            if (vector origin; spec.readIfPresent("origin", origin))
            {
                // Single zone specification
                const auto& coeffs = spec;

                origin_.resize_nocopy(1);
                omega_.resize_nocopy(1);
                zoneName_.resize_nocopy(1);

                origin_[0] = origin;
                omega_[0] = getSpecifiedOmega(coeffs);

                auto& zname = zoneName_[0];
                if (!coeffs.readIfPresent("zone", zname))
                {
                    zname.clear();
                }
            }
            else
            {
                // Multi-zone specification
                origin_.resize_nocopy(spec.size());
                omega_.resize_nocopy(spec.size());
                zoneName_.resize_nocopy(spec.size());

                count = 0;

                for (const auto& e : spec)
                {
                    if (const auto* dictptr = e.dictPtr())
                    {
                        const auto& key = e.keyword();
                        const auto& coeffs = *dictptr;

                        origin_[count] = coeffs.get<vector>("origin");
                        omega_[count] = getSpecifiedOmega(coeffs);
                        auto& zname = zoneName_[count];
                        ++count;

                        // The zone name selector is either directly
                        // specified or taken from the entry keyword
                        if (!coeffs.readIfPresent("zone", zname))
                        {
                            zname = wordRe(key);
                        }
                    }
                    else
                    {
                        WarningInFunction
                            << "Ignoring non-dictionary entry: "
                            << e.keyword() << endl;
                    }
                }
            }

            if (!count)
            {
                FatalIOErrorInFunction(dict)
                    << "No entries in the 'specified' sub-dictionary!"
                    << exit(FatalIOError);
            }

            origin_.resize(count);
            omega_.resize(count);
            zoneName_.resize(count);

            break;
        }

        case rotationMode::MRF :
        {
            const auto* ptr =
                mesh_.cfindObject<IOMRFZoneList>("MRFProperties");

            if (!ptr)
            {
                FatalErrorInFunction
                    << "Unable to find MRFProperties in the database.";

                if (functionObject::postProcess)
                {
                    FatalError
                        << nl << "Fragile behaviour in post-process mode.";
                }
                else
                {
                    FatalError
                        << " Is this an MRF case?";
                }

                // NB: FatalError not FatalIOError (for try/catch handling)
                FatalError << exit(FatalError);
            }

            MRFlist_ = static_cast<const MRFZoneList*>(ptr);
            break;
        }

        case rotationMode::SOLID_BODY_ROTATION :
        {
            sbmSolver_ =
                mesh_.cfindObject<solidBodyMotionSolver>("dynamicMeshDict");

            bool failed = false;

            if (!sbmSolver_)
            {
                failed = true;
                FatalErrorInFunction
                    << "SolidBodyMotion not found in the database.";
            }
            else
            {
                rotatingPtr_ =
                    isA<solidBodyMotionFunctions::rotatingMotion>
                    (
                        sbmSolver_->motionControl()
                    );

                if (!rotatingPtr_)
                {
                    failed = true;
                    FatalErrorInFunction
                        << "SolidBodyMotion is "
                        << sbmSolver_->motionControl().type()
                        << " but only rotatingMotion is supported.";
                }
            }

            if (failed)
            {
                if (functionObject::postProcess)
                {
                    FatalError
                        << nl << "Fragile behaviour in post-process mode.";
                }

                // NB: FatalError not FatalIOError (for try/catch handling)
                FatalError<< exit(FatalError);
            }
            break;
        }

        default :
        {
            FatalErrorInFunction
                << "Unhandled enumeration "
                << rotationModeNames_[rotationMode_]
                << abort(FatalError);
        }
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::
relativeVelocityFunctionObject::relativeVelocityFunctionObject
(
    const word& name,
    const Time& runTime,
    const dictionary& dict
)
:
    fvMeshFunctionObject(name, runTime, dict),
    backgroundMode_(backgroundMode::ZERO),
    rotationMode_(rotationMode::SPECIFIED),
    backgroundVelocity_(vector::zero),
    UName_("U"),
    resultName_("U_relative"),
    // For specified:
    origin_(),
    omega_(),
    zoneName_(),
    // For MRF:
    MRFlist_(nullptr),
    // For solidBodyRotation:
    sbmSolver_(nullptr),
    rotatingPtr_(nullptr)
{
    read(dict);
    Log << endl;
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::functionObjects::relativeVelocityFunctionObject::read
(
    const dictionary& dict
)
{
    if (fvMeshFunctionObject::read(dict))
    {
        backgroundMode_ = backgroundModeNames_.getOrDefault
        (
            "backgroundMode",
            dict,
            backgroundMode::ZERO,
            true  // Warn (not fail) on bad enumeration
        );

        if (backgroundMode_ == backgroundMode::SPECIFIED)
        {
            backgroundVelocity_ = dict.get<vector>("refVelocity");
        }
        else
        {
            backgroundVelocity_ = vector::zero;
        }

        rotationMode_ = rotationModeNames_.get("rotationMode", dict);

        UName_ = dict.getOrDefault<word>("U", "U");
        resultName_ = dict.getOrDefault<word>("result", "U_relative");

        init(dict);

        Info<< type() << " " << name() << ":" << nl
            << "    mode:" << rotationModeNames_[rotationMode_] << nl
            << "    background:" << backgroundModeNames_[backgroundMode_] << nl;

        return true;
    }

    return false;
}


bool Foam::functionObjects::relativeVelocityFunctionObject::execute()
{
    const auto& cc = mesh_.C().primitiveField();
    const auto& U = mesh_.lookupObject<volVectorField>(UName_);

    // Get existing or create new
    auto* prelVel = mesh_.getObjectPtr<volVectorField>(resultName_);
    if (!prelVel)
    {
        prelVel = new volVectorField
        (
            IOobject
            (
                resultName_,
                mesh_.time().timeName(),
                mesh_.thisDb(),
                IOobjectOption::NO_READ,
                IOobjectOption::NO_WRITE,
                IOobjectOption::REGISTER
            ),
            mesh_,
            U.dimensions(),  // NB: construct without value (assigned below)
            fvPatchFieldBase::zeroGradientType()
        );
        prelVel->store();
    }
    auto& relVel = *prelVel;

    const auto& Ucell = U.primitiveField();
    auto& relInternal = relVel.primitiveFieldRef();

    switch (backgroundMode_)
    {
        case backgroundMode::SPECIFIED :
        {
            relInternal = backgroundVelocity_;
            break;
        }
        case backgroundMode::COPY :
        {
            relInternal = Ucell;
            break;
        }
        default :  // backgroundMode::ZERO
        {
            relInternal = Foam::zero{};
            break;
        }
    }

    const auto relativeVelocity =
        [&Ucell,&cc](label celli, const vector& origin, const vector& omega)
        {
            return Ucell[celli] - (omega ^ (cc[celli] - origin));
        };

    // Often need these quantities
    const auto& allCellZones = mesh_.cellZones();
    const label nCells = mesh_.nCells();

    // Additional sanity checks (especially for postProcess mode!)
    switch (rotationMode_)
    {
        case rotationMode::MRF :
        {
            if (!MRFlist_)
            {
                WarningInFunction
                    << "MRF zones are not set" << endl;
                return false;
            }
            break;
        }

        case rotationMode::SOLID_BODY_ROTATION :
        {
            if (!sbmSolver_)
            {
                WarningInFunction
                    << "SolidBodyMotion not set" << endl;
                return false;
            }
            if (!rotatingPtr_)
            {
                WarningInFunction
                    << "SolidBodyRotation not set" << endl;
                return false;
            }
        }

        default :
        {
            break;
        }
    }

    switch (rotationMode_)
    {
        case rotationMode::SPECIFIED :
        {
            // Number of specified origin/axis/omega sets to apply
            const label numSpecified = origin_.size();

            for (label speci = 0; speci < numSpecified; ++speci)
            {
                const auto& origin = origin_[speci];
                const auto& omega = omega_[speci];
                const auto& zname = zoneName_[speci];

                if (zname.empty())
                {
                    for (label celli = 0; celli < nCells; ++celli)
                    {
                        relInternal[celli] =
                            relativeVelocity(celli, origin, omega);
                    }
                }
                else
                {
                    const labelList zoneIDs
                    (
                        allCellZones.indices(zname)
                    );

                    if (zoneIDs.empty())
                    {
                        WarningInFunction
                            << "No cellZones matches for name: "
                            << zname << endl;
                    }

                    for (label zoneID : zoneIDs)
                    {
                        const auto& cells = allCellZones[zoneID];

                        for (label celli : cells)
                        {
                            relInternal[celli] =
                                relativeVelocity(celli, origin, omega);
                        }
                    }
                }
            }

            break;
        }

        case rotationMode::MRF :
        {
            bool applied = false;

            for (const auto& mrf : *MRFlist_)
            {
                if
                (
                    const cellZone* czptr = mrf.whichZone();
                    (bool(czptr) && mrf.active())
                )
                {
                    applied = true;

                    const auto& origin = mrf.origin();
                    const auto& omega = mrf.Omega();
                    const auto& cells = *czptr;

                    for (label celli : cells)
                    {
                        relInternal[celli] =
                            relativeVelocity(celli, origin, omega);
                    }
                }
            }

            if (!applied)
            {
                WarningInFunction
                    << "No MRF zones applied" << endl;
            }

            break;
        }

        case rotationMode::SOLID_BODY_ROTATION :
        {
            // Get cellZone information from
            // solidBodyMotionSolver < points0MotionSolver < zoneMotion

            const auto& znMotion =
                static_cast<const zoneMotion&>(*sbmSolver_);

            // Get motion (origin/axis/omega) from rotationMode
            const auto& origin = (*rotatingPtr_).origin();
            const auto& omega = (*rotatingPtr_).Omega();

            if (znMotion.moveAllCells())
            {
                for (label celli = 0; celli < nCells; ++celli)
                {
                    relInternal[celli] =
                        relativeVelocity(celli, origin, omega);
                }
            }
            else
            {
                bool applied = false;

                for (label zoneID : znMotion.cellZoneIDs())
                {
                    applied = true;

                    const auto& cells = allCellZones[zoneID];

                    for (label celli : cells)
                    {
                        relInternal[celli] =
                            relativeVelocity(celli, origin, omega);
                    }
                }

                if (!applied)
                {
                    WarningInFunction
                        << "No solidBodyMotion zones applied" << endl;
                }
            }

            break;
        }

        default :
        {
            FatalErrorInFunction
                << "Unhandled enumeration "
                << rotationModeNames_[rotationMode_]
                << abort(FatalError);

            break;
        }
    }

    relVel.correctBoundaryConditions();

    return true;
}


bool Foam::functionObjects::relativeVelocityFunctionObject::write()
{
    if (const auto* fld = mesh_.cfindObject<volVectorField>(resultName_))
    {
        Log << type() << ' ' << name() << " write: " << fld->name() << endl;
        fld->write();
    }

    return true;
}


// ************************************************************************* //
