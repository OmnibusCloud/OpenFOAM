/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2019-2026 OpenCFD Ltd.
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

#include "GeometricFieldExpression.H"
#include "volFieldsFwd.H"
#include "fvMatrixExpression.H"
#include "incompressibleKEpsilon.H"
#include "fvOptions.H"
#include "bound.H"
#include "addToRunTimeSelectionTable.H"
#include "wallDist.H"

// #include <ratio>
// #include <chrono>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace incompressible
{
namespace RASModels
{

//- Wrap of constant as a list expression
typedef Expression::UniformListWrap<scalar> constant;
//- Wrap of List as an expression
typedef Expression::ListConstRefWrap<scalar> expr;
//- Evaluator of an expression
typedef Expression::ListRefWrap<scalar> evaluator;


//- Wrap of constant as an expression
typedef Expression::UniformGeometricFieldWrap<volScalarField> volConstant;
//- Wrap of volScalarField as an expression
typedef Expression::GeometricFieldConstRefWrap<volScalarField> volExpr;
//- Evaluator of an expression
typedef Expression::GeometricFieldRefWrap<volScalarField> volEvaluator;


// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(incompressibleKEpsilon, 0);
addToRunTimeSelectionTable(RASModel, incompressibleKEpsilon, dictionary);

// static double time_total = 0;

// * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * * //

void incompressibleKEpsilon::correctNut()
{
    //this->nut_ = Cmu_*sqr(k_)/epsilon_;

    // Create 'oneField' from Cmu
    const volConstant Cmu(this->nut_, Cmu_);

    volEvaluator(this->nut_).evaluate
    (
        Cmu
      * sqr(volExpr(k_))
      / volExpr(epsilon_)
    );

    if (twoLayerTreatment_)
    {
        const volScalarField& y = wallDist::New(this->mesh_).y();

        // Using the TKE of previous time-step to freeze the turbulent Reynolds
        // number during the current time-step and hence freeze the separation
        // of inner and outer layer.
        const volScalarField Rey(mag(y*sqrt(k_.oldTime())/this->nu()));

        const dimensionedScalar ClStar(0.41*pow(Cmu_, -0.75));
        auto& lEps = *lEpsPtr_;
        lEps = y*ClStar*(1.0 - exp(-1.0*Rey/(2*ClStar)));

        const dimensionedScalar A(ReyFactor_*ReyStar_/atanh(0.98));
        auto& lambdaEps = *lambdaEpsPtr_;
        lambdaEps = 0.5*(1.0 + tanh((Rey - ReyStar_)/A));

        // High-Re part
        this->nut_ *= lambdaEps;

        // Low-Re part
        const scalar Amu = 70.0;
        const volScalarField lMu(y*ClStar*(1.0 - exp(-1.0*Rey/Amu)));
        this->nut_ += (1.0-lambdaEps)*Cmu_*lMu*sqrt(k_);
    }

    this->nut_.correctBoundaryConditions();
    fv::options::New(this->mesh_).correct(this->nut_);

    IncompressibleTurbulenceModel<Foam::transportModel>::correctNut();
}


tmp<fvScalarMatrix> incompressibleKEpsilon::kSource() const
{
    // Source term for k equation (added to RHS)

    if (twoLayerTreatment_)
    {
        // Local references
        const alphaField& alpha = this->alpha_;
        const rhoField& rho = this->rho_;
        const volScalarField::Internal& lEps = *lEpsPtr_;
        const volScalarField& lambdaEps = *lambdaEpsPtr_;

        return
           -fvm::Sp
            (
                alpha()*rho()*(1.0-lambdaEps())
               *(sqrt(k_())/lEps - epsilon_()/k_()),
                k_
            );
    }

    return tmp<fvScalarMatrix>::New
    (
        k_,
        dimVolume*this->rho_.dimensions()*k_.dimensions()/dimTime
    );
}


tmp<fvScalarMatrix> incompressibleKEpsilon::epsilonSource() const
{
    return tmp<fvScalarMatrix>::New
    (
        epsilon_,
        dimVolume*this->rho_.dimensions()*epsilon_.dimensions()/dimTime
    );
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

incompressibleKEpsilon::incompressibleKEpsilon
(
    const geometricOneField& alpha,
    const geometricOneField& rho,
    const volVectorField& U,
    const surfaceScalarField& alphaRhoPhi,
    const surfaceScalarField& phi,
    const transportModel& transport,
    const word& propertiesName,
    const word& type
)
:
    eddyViscosity<incompressible::RASModel>
    (
        type,
        alpha,
        rho,
        U,
        alphaRhoPhi,
        phi,
        transport,
        propertiesName
    ),

    Cmu_
    (
        dimensioned<scalar>::getOrAddToDict
        (
            "Cmu",
            this->coeffDict_,
            0.09
        )
    ),
    C1_
    (
        dimensioned<scalar>::getOrAddToDict
        (
            "C1",
            this->coeffDict_,
            1.44
        )
    ),
    C2_
    (
        dimensioned<scalar>::getOrAddToDict
        (
            "C2",
            this->coeffDict_,
            1.92
        )
    ),
    C3_
    (
        dimensioned<scalar>::getOrAddToDict
        (
            "C3",
            this->coeffDict_,
            0
        )
    ),
    sigmak_
    (
        dimensioned<scalar>::getOrAddToDict
        (
            "sigmak",
            this->coeffDict_,
            1.0
        )
    ),
    sigmaEps_
    (
        dimensioned<scalar>::getOrAddToDict
        (
            "sigmaEps",
            this->coeffDict_,
            1.3
        )
    ),
    k_
    (
        IOobject
        (
            IOobject::groupName("k", alphaRhoPhi.group()),
            this->runTime_.timeName(),
            this->mesh_,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        this->mesh_
    ),
    epsilon_
    (
        IOobject
        (
            IOobject::groupName("epsilon", alphaRhoPhi.group()),
            this->runTime_.timeName(),
            this->mesh_,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        this->mesh_
    ),
    twoLayerTreatment_
    (
        Switch::getOrAddToDict
        (
            "twoLayerTreatment",
            this->coeffDict_,
            false
        )
    ),
    ReyStar_
    (
        dimensioned<scalar>::getOrAddToDict
        (
            "ReyStar",
            this->coeffDict_,
            200.0
        )
    ),
    ReyFactor_
    (
        dimensioned<scalar>::getOrAddToDict
        (
            "ReyFactor",
            this->coeffDict_,
            0.05
        )
    )
{
    if (twoLayerTreatment_)
    {
        Info<< "Two-layer wall treatment activated" << endl;

        lEpsPtr_ = std::make_unique<volScalarField::Internal>
        (
            IOobject
            (
                "lEps",
                this->runTime_.name(),
                this->mesh_,
                IOobject::READ_IF_PRESENT,
                IOobject::AUTO_WRITE
            ),
            this->mesh_,
            dimensionedScalar(dimLength, Zero)
        );

        lambdaEpsPtr_ = std::make_unique<volScalarField>
        (
            IOobject
            (
                "lambdaEps",
                this->runTime_.name(),
                this->mesh_,
                IOobject::READ_IF_PRESENT,
                IOobject::AUTO_WRITE
            ),
            this->mesh_,
            dimensionedScalar("lambdaEps", dimless, 1.0)
        );
    }

    bound(k_, this->kMin_);
    bound(epsilon_, this->epsilonMin_);

    if (type == typeName)
    {
        this->printCoeffs(type);
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool incompressibleKEpsilon::read()
{
    if (eddyViscosity<incompressible::RASModel>::read())
    {
        Cmu_.readIfPresent(this->coeffDict());
        C1_.readIfPresent(this->coeffDict());
        C2_.readIfPresent(this->coeffDict());
        C3_.readIfPresent(this->coeffDict());
        sigmak_.readIfPresent(this->coeffDict());
        sigmaEps_.readIfPresent(this->coeffDict());

        // Two-layer wall treatment
        // Note: these are the only parameters that can be changed at runtime
        ReyStar_.readIfPresent(this->coeffDict());
        ReyFactor_.readIfPresent(this->coeffDict());

        return true;
    }

    return false;
}


void incompressibleKEpsilon::correct()
{
    if (!this->turbulence_)
    {
        return;
    }
    // const std::chrono::high_resolution_clock::time_point t1 =
    //     std::chrono::high_resolution_clock::now();

    // Sizing
    const label nCells = this->nut_.size();

    // Create some expression constants
    const constant alpha(nCells, 1.0);
    const constant rho(nCells, 1.0);
    const constant Cmu(nCells, Cmu_);
    const constant C1(nCells, C1_);
    const constant C2(nCells, C2_);
    const constant C3(nCells, C3_);
    const constant C1C3(nCells, (2.0/3.0)*C1_ - C3_);
    const constant twoThird(nCells, (2.0/3.0));
    const constant minOne(nCells, -1.0);
    //const constant one(nCells, 1.0);

    const surfaceScalarField& alphaRhoPhi = this->alphaRhoPhi_;
    const volVectorField& U = this->U_;
    const volScalarField& nut = this->nut_;

    fv::options& fvOptions(fv::options::New(this->mesh_));

    eddyViscosity<incompressible::RASModel>::correct();

    const volScalarField::Internal divU
    (
        fvc::div(fvc::absolute(this->phi(), U))().v()
    );

    tmp<volTensorField> tgradU = fvc::grad(U);
    const volScalarField::Internal GbyNu
    (
        IOobject::scopedName(this->type(), "GbyNu"),
        tgradU().v() && devTwoSymm(tgradU().v())
    );
    const volScalarField::Internal G(this->GName(), nut()*GbyNu);
    tgradU.clear();

    // Update epsilon and G at the wall
    epsilon_.boundaryFieldRef().updateCoeffs();
    // Push any changed cell values to coupled neighbours
    epsilon_.boundaryFieldRef().template evaluateCoupled<coupledFvPatch>();


    // Wrap internal fields as list expressions for convenience
    const expr wepsilon(epsilon_.internalField());
    const expr wk(k_.internalField());
    const expr V(k_.mesh().V());


    // Dissipation equation
    // ~~~~~~~~~~~~~~~~~~~~

    // Wrap zero
    const constant constantZero(V.size(), 0.0);

    // Dissipation equation
    tmp<fvScalarMatrix> epsEqn
    (
        fvm::ddt(epsilon_)
      + fvm::div(alphaRhoPhi, epsilon_)
      - fvm::laplacian(DepsilonEff(), epsilon_)
     ==
        //   C1_*alpha()*rho()*GbyNu*Cmu_*k_()
        // - fvm::SuSp(((2.0/3.0)*C1_ - C3_)*alpha()*rho()*divU, epsilon_)
        // - fvm::Sp(C2_*alpha()*rho()*epsilon_()/k_(), epsilon_)
        epsilonSource()
      + fvOptions(epsilon_)
    );
    {
        // Implement source terms as expression templates.
        auto& m = epsEqn.ref();

        m =
            (
                m.expr()    // Get matrix in fvMatrixExpression form
            ==
                Expression::Su(C1*alpha*rho*GbyNu*Cmu*k_, m)
              - Expression::SuSp(C1C3*alpha*rho*divU, m)
              - Expression::Sp(C2*alpha*rho*epsilon_/k_, m)
            );

        // Alternative using field. Assumes dummy dimensions.
        // epsEqn.ref() =
        //     (
        //         epsEqn().expr()
        //      ==
        //         Expression::Su(C1*alpha*rho*GbyNu*Cmu*k_, epsilon_)
        //       - Expression::SuSp(C1C3*alpha*rho*divU, epsilon_)
        //       - Expression::Sp(C2*alpha*rho*epsilon_/k_, epsilon_)
        //     );
    }



    epsEqn.ref().relax();
    fvOptions.constrain(epsEqn.ref());
    epsEqn.ref().boundaryManipulate(epsilon_.boundaryFieldRef());
    solve(epsEqn);
    fvOptions.correct(epsilon_);
    bound(epsilon_, this->epsilonMin_);


    // Turbulent kinetic energy equation
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    tmp<fvScalarMatrix> kEqn
    (
        fvm::ddt(k_)
      + fvm::div(alphaRhoPhi, k_)
      - fvm::laplacian(DkEff(), k_)
     ==
    //     G
    //   - fvm::SuSp((2.0/3.0)*divU, k_)
    //   - fvm::Sp(epsilon_()/k_(), k_)
        kSource()
      + fvOptions(k_)
    );
    {
        // Implement source terms as expression templates.
        auto& m = kEqn.ref();

        m =
            (
                m.expr()
             ==
                Expression::Su(alpha*rho*G, m)
              - Expression::SuSp(twoThird*alpha*rho*divU, m)
              - Expression::Sp(alpha*rho*epsilon_/k_, m)
            );
    }

    kEqn.ref().relax();
    fvOptions.constrain(kEqn.ref());
    solve(kEqn);
    fvOptions.correct(k_);
    bound(k_, this->kMin_);

    correctNut();

    // const std::chrono::high_resolution_clock::time_point t2 =
    //     std::chrono::high_resolution_clock::now();
    // const std::chrono::duration<double> time_span = t2 - t1;
    // time_total += time_span.count();
    // Info<< "incompressibleKEpsilon: spent:" << time_span.count()
    //     << " total:" << time_total << endl;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace RASModels
} // End namespace incompressible
} // End namespace Foam

// ************************************************************************* //
