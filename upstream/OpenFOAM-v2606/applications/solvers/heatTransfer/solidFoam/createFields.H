Info<< "Reading thermophysical properties\n" << endl;

autoPtr<solidThermo> pThermo(solidThermo::New(mesh));
solidThermo& thermo = pThermo();

tmp<volScalarField> trho = thermo.rho();

IOobject betavSolidIO
(
    "betavSolid",
    runTime.timeName(),
    mesh,
    IOobject::MUST_READ,
    IOobject::AUTO_WRITE
);

autoPtr<volScalarField> betavPtr;
if (betavSolidIO.typeHeaderOk<volScalarField>(true))
{
    betavPtr.reset
    (
        new volScalarField
        (
            betavSolidIO,
            mesh
        )
    );
}
else
{
    betavPtr.reset
    (
        new volScalarField
        (
            IOobject
            (
                "betavSolid",
                runTime.timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh,
            dimensionedScalar("1", dimless, scalar(1))
        )
    );
}

// Consider mesh flux to correct for mesh deformation
bool meshFluxCorr(false);
if (mesh.solutionDict().found("SIMPLE"))
{
    meshFluxCorr =
        mesh.solutionDict().subDict("SIMPLE").getOrDefault<bool>
        (
            "meshFluxCorrection", false
        );
}
else if (mesh.solutionDict().found("PIMPLE"))
{
    meshFluxCorr =
        mesh.solutionDict().subDict("PIMPLE").getOrDefault<bool>
        (
            "meshFluxCorrection", false
        );
}

#include "createRadiationModel.H"
#include "createFvOptions.H"
