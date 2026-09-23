/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2018-2021 OpenFOAM Foundation
    Copyright (C) 2021-2025 OpenCFD Ltd.
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

#include "age.H"
#include "fvmDiv.H"
#include "fvmLaplacian.H"
#include "fvOptions.H"
#include "turbulentTransportModel.H"
#include "turbulentFluidThermoModel.H"
#include "turbulenceModel.H"
#include "inletOutletFvPatchField.H"
#include "wallFvPatch.H"
#include "zeroGradientFvPatchField.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
    defineTypeNameAndDebug(age, 0);
    addToRunTimeSelectionTable(functionObject, age, dictionary);
}
}

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

Foam::wordList Foam::functionObjects::age::patchTypes() const
{
    const fvBoundaryMesh& patches = mesh_.boundary();

    wordList result
    (
        patches.size(),
        inletOutletFvPatchField<scalar>::typeName
    );

    forAll(patches, patchi)
    {
        if (isA<wallFvPatch>(patches[patchi]))
        {
            result[patchi] = fvPatchFieldBase::zeroGradientType();
        }
    }

    return result;
}


bool Foam::functionObjects::age::converged
(
    const int nCorr,
    const scalar initialResidual
) const
{
    if (initialResidual < tolerance_)
    {
        Info<< "Field " << typeName
            << " converged in " << nCorr << " correctors"
            << nl << endl;

        return true;
    }

    return false;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::age::age
(
    const word& name,
    const Time& runTime,
    const dictionary& dict
)
:
    fvMeshFunctionObject(name, runTime, dict)
{
    read(dict);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::functionObjects::age::read(const dictionary& dict)
{
    if (!fvMeshFunctionObject::read(dict))
    {
        return false;
    }

    phiName_ = dict.getOrDefault<word>("phi", "phi");
    rhoName_ = dict.getOrDefault<word>("rho", "rho");
    schemesField_ = dict.getOrDefault<word>("schemesField", typeName);
    tolerance_ = dict.getOrDefault<scalar>("tolerance", 1e-5);
    nCorr_ = dict.getOrDefault<int>("nCorr", 5);
    diffusion_ = dict.getOrDefault<bool>("diffusion", false);


    // Detect if compressible or incompressible
    const auto& phi = mesh_.lookupObject<surfaceScalarField>(phiName_);
    isCompressible_ = (phi.dimensions() == dimMass/dimTime);

    // Store the divergence scheme
    divScheme_ = word("div(phi," + schemesField_ + ")");

    // Store the Laplacian scheme
    if (diffusion_)
    {
        if (isCompressible_)
        {
            laplacianScheme_ = "laplacian(muEff," + schemesField_ + ")";
        }
        else
        {
            laplacianScheme_ = "laplacian(nuEff," + schemesField_ + ")";
        }
    }

    return true;
}


bool Foam::functionObjects::age::execute()
{
    auto* agePtr = getObjectPtr<volScalarField>(typeName);
    if (!agePtr)
    {
        agePtr = new volScalarField
        (
            IOobject
            (
                typeName,
                obr().time().timeName(),
                obr(),
                IOobject::READ_IF_PRESENT,
                IOobject::AUTO_WRITE,
                IOobject::NO_REGISTER
            ),
            mesh_,
            dimensionedScalar(dimTime, Zero),
            patchTypes()
        );
        regIOobject::store(agePtr);
    }
    auto& age = *agePtr;


    // Set under-relaxation coeff
    scalar relaxCoeff = 0;
    mesh_.relaxEquation(schemesField_, relaxCoeff);

    Foam::fv::options& fvOptions(Foam::fv::options::New(mesh_));


    // This only works because the null constructed inletValue for an
    // inletOutletFvPatchField is zero. If we needed any other value we would
    // have to loop over the inletOutlet patches and explicitly set the
    // inletValues. We would need to change the interface of inletOutlet in
    // order to do this.

    const auto& phi = mesh_.lookupObject<surfaceScalarField>(phiName_);

    if (isCompressible_)
    {
        const auto& rho = mesh_.lookupObject<volScalarField>(rhoName_);

        tmp<volScalarField> tmuEff;

        if (diffusion_)
        {
            tmuEff =
                mesh_.lookupObject<compressible::turbulenceModel>
                (
                    turbulenceModel::propertiesName
                ).muEff();
        }

        for (int i = 0; i <= nCorr_; ++i)
        {
            fvScalarMatrix ageEqn
            (
                fvm::div(phi, age, divScheme_) == rho //+ fvOptions(rho, age)
            );

            if (diffusion_)
            {
                ageEqn -= fvm::laplacian(tmuEff(), age, laplacianScheme_);
            }

            ageEqn.relax(relaxCoeff);

            fvOptions.constrain(ageEqn);

            if (converged(i, ageEqn.solve().initialResidual()))
            {
                break;
            }

            fvOptions.correct(age);
        }
    }
    else
    {
        tmp<volScalarField> tnuEff;

        if (diffusion_)
        {
            tnuEff =
                mesh_.lookupObject<incompressible::turbulenceModel>
                (
                    turbulenceModel::propertiesName
                ).nuEff();
        }

        for (int i = 0; i <= nCorr_; ++i)
        {
            fvScalarMatrix ageEqn
            (
                fvm::div(phi, age, divScheme_)
             == dimensionedScalar(1) + fvOptions(age)
            );

            if (diffusion_)
            {
                ageEqn -= fvm::laplacian(tnuEff(), age, laplacianScheme_);
            }

            ageEqn.relax(relaxCoeff);

            fvOptions.constrain(ageEqn);

            if (converged(i, ageEqn.solve().initialResidual()))
            {
                break;
            }

            fvOptions.correct(age);
        }
    }

    Info<< "Min/max age:"
        << min(age).value() << ' '
        << max(age).value()
        << endl;

    return true;
}


bool Foam::functionObjects::age::write()
{
    return writeObject(typeName);
}


// ************************************************************************* //
