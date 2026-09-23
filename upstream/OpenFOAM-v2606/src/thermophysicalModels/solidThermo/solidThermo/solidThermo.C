/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2012 OpenFOAM Foundation
    Copyright (C) 2017 OpenCFD Ltd.
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

#include "solidThermo.H"
#include "fvMesh.H"
#include "fvm.H"
#include "fvc.H"
#include "surfaceFields.H"
#include "addToRunTimeSelectionTable.H"


/* * * * * * * * * * * * * * * private static data * * * * * * * * * * * * * */

namespace Foam
{
    defineTypeNameAndDebug(solidThermo, 0);
    defineRunTimeSelectionTable(solidThermo, fvMesh);
    defineRunTimeSelectionTable(solidThermo, dictionary);
    defineRunTimeSelectionTable(solidThermo, fvMeshDictPhase);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace
{

//- Compute harmonic-averaged face-normal alpha and kappa for
//  anisotropic zone mixtures.
//  Returns (αf, κf) where:
//    αf[face] = 1 / ((1-w)/a_own + w/a_nei)   (harmonic of n·aniAlpha·n)
//    κf[face] = 1 / ((1-w)/k_own + w/k_nei)   (harmonic of n·aniAlpha·Cp·n)
std::pair<tmp<surfaceScalarField>, tmp<surfaceScalarField>>
calcHarmonicCoeffs
(
    const fvMesh& mesh,
    const volSymmTensorField& aniAlpha,
    const volScalarField& cp
)
{
    IOobject ioAlpha
    (
        "alphaf",
        mesh.time().timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    );

    IOobject ioKappa
    (
        "kappaf",
        mesh.time().timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    );

    // Kappa = aniAlpha * cp  (for dimension checking)
    auto tKappa = aniAlpha*cp;
    const dimensionSet dimKappa = tKappa().dimensions();
    const dimensionSet dimAlpha = aniAlpha.dimensions();

    auto talphaf = tmp<surfaceScalarField>::New
    (
        ioAlpha,
        mesh,
        dimAlpha
    );
    auto tkappaf = tmp<surfaceScalarField>::New
    (
        ioKappa,
        mesh,
        dimKappa
    );

    auto& alphaf = talphaf.ref();
    auto& kappaf = tkappaf.ref();

    // Standard linear interpolation weights from the mesh
    const surfaceScalarField& w = mesh.surfaceInterpolation::weights();

    const labelUList& owner = mesh.owner();
    const labelUList& neighbour = mesh.neighbour();
    const surfaceVectorField& Sf = mesh.Sf();
    const surfaceScalarField& magSf = mesh.magSf();

    // Internal faces: distance-weighted harmonic mean
    for (label facei = 0; facei < mesh.nInternalFaces(); ++facei)
    {
        const vector n = Sf[facei] / magSf[facei];

        const scalar aOwn = n & aniAlpha[owner[facei]] & n;
        const scalar aNei = n & aniAlpha[neighbour[facei]] & n;
        const scalar kOwn = aOwn * cp[owner[facei]];
        const scalar kNei = aNei * cp[neighbour[facei]];

        const scalar wf = w[facei];  // weight toward neighbour

        alphaf[facei] = 1.0 / ((1.0 - wf)/aOwn + wf/aNei + VSMALL);
        kappaf[facei] = 1.0 / ((1.0 - wf)/kOwn + wf/kNei + VSMALL);
    }

    // Boundary faces
    forAll(alphaf.boundaryField(), patchi)
    {
        const fvPatch& patch = mesh.boundary()[patchi];
        const vectorField n(patch.nf());

        auto& alphab = alphaf.boundaryFieldRef()[patchi];
        auto& kappab = kappaf.boundaryFieldRef()[patchi];

        if (aniAlpha.boundaryField()[patchi].coupled())
        {
            // Coupled patch (processor, cyclic, etc.): harmonic average
            // using both sides, consistent with the internal-face treatment.
            const tmp<symmTensorField> taniOwn =
                aniAlpha.boundaryField()[patchi].patchInternalField();
            const tmp<symmTensorField> taniNei =
                aniAlpha.boundaryField()[patchi].patchNeighbourField();
            const tmp<scalarField> tcpOwn =
                cp.boundaryField()[patchi].patchInternalField();
            const tmp<scalarField> tcpNei =
                cp.boundaryField()[patchi].patchNeighbourField();
            const scalarField& pw = patch.weights();

            forAll(patch, facei)
            {
                const scalar aOwn = n[facei] & taniOwn()[facei] & n[facei];
                const scalar aNei = n[facei] & taniNei()[facei] & n[facei];
                const scalar kOwn = aOwn * tcpOwn()[facei];
                const scalar kNei = aNei * tcpNei()[facei];
                const scalar wf = pw[facei];

                alphab[facei] = 1.0 / ((1.0 - wf)/aOwn + wf/aNei + VSMALL);
                kappab[facei] = 1.0 / ((1.0 - wf)/kOwn + wf/kNei + VSMALL);
            }
        }
        else
        {
            // Physical boundary: one-sided projection from the adjacent cell
            const labelUList& faceCells = patch.faceCells();

            forAll(patch, facei)
            {
                const label celli = faceCells[facei];
                const scalar an = n[facei] & aniAlpha[celli] & n[facei];

                alphab[facei] = an;
                kappab[facei] = an * cp[celli];
            }
        }
    }

    return {std::move(talphaf), std::move(tkappaf)};
}

} // End anonymous namespace
} // End namespace Foam


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::solidThermo::solidThermo
(
    const fvMesh& mesh,
    const word& phaseName
)
:
    rhoThermo(mesh, phaseName)
{}


Foam::solidThermo::solidThermo
(
    const fvMesh& mesh,
    const dictionary& dict,
    const word& phaseName
)
:
    rhoThermo(mesh, dict, phaseName)
{}


Foam::solidThermo::solidThermo
(
    const fvMesh& mesh,
    const word& phaseName,
    const word& dictionaryName
)
:
     rhoThermo(mesh, phaseName, dictionaryName)
{}



// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

Foam::autoPtr<Foam::solidThermo> Foam::solidThermo::New
(
    const fvMesh& mesh,
    const word& phaseName
)
{
    return basicThermo::New<solidThermo>(mesh, phaseName);
}


Foam::autoPtr<Foam::solidThermo> Foam::solidThermo::New
(
    const fvMesh& mesh,
    const dictionary& dict,
    const word& phaseName
)
{
    return basicThermo::New<solidThermo>(mesh, dict, phaseName);
}


Foam::autoPtr<Foam::solidThermo> Foam::solidThermo::New
(
     const fvMesh& mesh,
     const word& phaseName,
     const word& dictName
)
{
    return basicThermo::New<solidThermo>(mesh, phaseName, dictName);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::solidThermo::~solidThermo()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::tmp<Foam::volScalarField> Foam::solidThermo::rho() const
{
    return rho_;
}


Foam::tmp<Foam::scalarField> Foam::solidThermo::rho(const label patchi) const
{
    return rho_.boundaryField()[patchi];
}


Foam::volScalarField& Foam::solidThermo::rho()
{
    return rho_;
}


bool Foam::solidThermo::read()
{
    return regIOobject::read();
}


Foam::tmp<Foam::fvScalarMatrix> Foam::solidThermo::heatDiffusion
(
    const volScalarField& betav,
    const volScalarField& h
) const
{
    if (!isZoneMixture())
    {
        // Standard case: single implicit laplacian
        if (isotropic())
        {
            return fvm::laplacian(betav*alpha(), h, "laplacian(alpha,h)");
        }
        else
        {
            return fvm::laplacian(betav*aniAlpha(), h, "laplacian(alpha,h)");
        }
    }
    else
    {
        // Zone mixture: add explicit correction terms.
        // At convergence, fvm::laplacian(alpha, h) and
        // fvc::laplacian(alpha, h) cancel, leaving
        // -fvc::laplacian(kappa, T), which is the correct diffusion.
        if (isotropic())
        {
            return
                fvm::laplacian(betav*alpha(), h, "laplacian(alpha,h)")
              - fvc::laplacian(betav*alpha(), h, "laplacian(alpha,h)")
              + fvc::laplacian(betav*kappa(), T(), "laplacian(kappa,T)");
        }
        else
        {
            // Hybrid approach: harmonic normal-component + full tensor coupling
            //
            // The orthogonal face flux uses the harmonic mean of n·A·n for
            // correct behaviour at material interfaces.  The non-orthogonal
            // correction uses the full face tensor (linearly interpolated) to
            // retain off-diagonal tensor coupling.
            //
            // Construction of the corrected face tensor:
            //   A_f_corr = A_f_lin + (alphaf_harm - n·A_f_lin·n) · (n ⊗ n)
            //
            // This replaces only the normal-normal component with the harmonic
            // value while preserving all off-diagonal entries.

            const fvMesh& mesh = h.mesh();
            const volSymmTensorField& aniAlpha = this->aniAlpha();

            tmp<volScalarField> tCp = this->Cp();
            const volScalarField& cp = tCp();

            // 1. Harmonic scalar for n·A·n (correct at interfaces)
            auto [alphaf_harm, kappaf_harm] =
                calcHarmonicCoeffs(mesh, aniAlpha, cp);

            // 2. Linearly-interpolated full tensors at faces
            const surfaceSymmTensorField A_f_lin(fvc::interpolate(aniAlpha));

            const surfaceSymmTensorField kappa_f_lin
            (
                 fvc::interpolate(aniAlpha*cp)
            );

            // 3. Face unit normal
            const surfaceVectorField n(mesh.Sf() / mesh.magSf());

            // 4. Normal-normal component from linear interpolation
            const surfaceScalarField nnAlpha(n & A_f_lin & n);

            // 5. Corrected face tensors
            surfaceSymmTensorField A_f_corr(A_f_lin);
            A_f_corr += (alphaf_harm - nnAlpha) * symm(n * n);

            surfaceSymmTensorField kappa_f_corr(kappa_f_lin);
            const surfaceScalarField nnKappa(n & kappa_f_lin & n);
            kappa_f_corr += (kappaf_harm - nnKappa) * symm(n * n);

            // 6. Apply porosity factor
            const surfaceScalarField betav_f(fvc::interpolate(betav));

            // 7. Full tensor laplacians — the standard
            //    gaussLaplacianScheme now sees SfGammaCorr ≠ 0,
            //    so the non-orthogonal correction correctly includes
            //    off-diagonal tensor coupling.  The nNonOrthogonalCorrectors
            //    loop in the solver operates as usual.
            return
                fvm::laplacian(betav_f * A_f_corr, h, "laplacian(alpha,h)")
              - fvc::laplacian(betav_f * A_f_corr, h, "laplacian(alpha,h)")
              + fvc::laplacian(betav_f * kappa_f_corr, T(), "laplacian(kappa,T)");
        }
    }
}


// ************************************************************************* //
