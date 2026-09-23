/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2015-2025 OpenCFD Ltd.
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

#include "LocalInteraction.H"
#include "BaiGosman.C"
#include "fvMesh.H"
#include "volFields.H"

// * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * * //

template<class CloudType>
void Foam::LocalInteraction<CloudType>::writeFileHeader(Ostream& os)
{
    PatchInteractionModel<CloudType>::writeFileHeader(os);

    forAll(nEscape_, patchi)
    {
        const word& patchName = patchData_[patchi].patchName();

        forAll(nEscape_[patchi], injectori)
        {
            const word suffix = Foam::name(injectori);
            this->writeTabbed(os, patchName + "_nEscape_" + suffix);
            this->writeTabbed(os, patchName + "_massEscape_" + suffix);
            this->writeTabbed(os, patchName + "_nStick_" + suffix);
            this->writeTabbed(os, patchName + "_massStick_" + suffix);

            if (hasBaiGosman_)
            {
                this->writeTabbed(os, patchName + "_nRebound_" + suffix);
                this->writeTabbed(os, patchName + "_massRebound_" + suffix);
                this->writeTabbed(os, patchName + "_nSplash_" + suffix);
                this->writeTabbed(os, patchName + "_massSplash_" + suffix);
            }
        }
    }
}


template<class CloudType>
Foam::LocalInteraction<CloudType>::LocalInteraction
(
    const dictionary& dict,
    CloudType& cloud
)
:
    PatchInteractionModel<CloudType>(dict, cloud, typeName),
    patchData_(cloud.mesh(), this->coeffDict()),
    nEscape_(patchData_.size()),
    massEscape_(nEscape_.size()),
    nStick_(nEscape_.size()),
    massStick_(nEscape_.size()),
    nRebound_(nEscape_.size()),
    massRebound_(nEscape_.size()),
    nSplash_(nEscape_.size()),
    massSplash_(nEscape_.size()),
    writeFields_(this->coeffDict().getOrDefault("writeFields", false)),
    hasBaiGosman_(false),
    beiGosman_(*this),
    injIdToIndex_(),
    massEscapePtr_(nullptr),
    massStickPtr_(nullptr)
{
    const bool outputByInjectorId
        = this->coeffDict().getOrDefault("outputByInjectorId", false);

    if (writeFields_)
    {
        Info<< "    Interaction fields will be written to "
            << IOobject::scopedName(this->owner().name(), "massEscape")
            << " and "
            << IOobject::scopedName(this->owner().name(), "massStick") << endl;

        (void)massEscape();
        (void)massStick();
    }
    else
    {
        Info<< "    Interaction fields will not be written" << endl;
    }

    // Determine the number of injectors and the injector mapping
    label nInjectors = 0;
    if (outputByInjectorId)
    {
        for (const auto& inj : cloud.injectors())
        {
            injIdToIndex_.insert(inj.injectorID(), nInjectors++);
        }
    }

    // The normal case, and safety if injector mapping was somehow null.
    if (!nInjectors)
    {
        nInjectors = 1;
    }

    hasBaiGosman_ = hasBaiGosmanPatch();

    if (hasBaiGosman_)
    {
        beiGosman_.initialise
        (
            localInteractionModels::parcelHasThermo
            <
                typename CloudType::parcelType
            >()
        );
    }

    // Check that interactions are valid/specified
    forAll(patchData_, patchi)
    {
        const word& interactionTypeName =
            patchData_[patchi].interactionTypeName();

        if (!patchData_[patchi].isBaiGosman())
        {
            const typename PatchInteractionModel<CloudType>::interactionType& it =
                this->wordToInteractionType(interactionTypeName);

            if (it == PatchInteractionModel<CloudType>::itOther)
            {
                const word& patchName = patchData_[patchi].patchName();
                FatalErrorInFunction
                    << "Unknown patch interaction type "
                    << interactionTypeName << " for patch " << patchName
                    << ". Valid selections are:"
                    << this->PatchInteractionModel<CloudType>::interactionTypeNames_
                    << " and " << patchInteractionData::beiGosmanTypeName
                    << nl << exit(FatalError);
            }
        }

        nEscape_[patchi].setSize(nInjectors, Zero);
        massEscape_[patchi].setSize(nInjectors, Zero);
        nStick_[patchi].setSize(nInjectors, Zero);
        massStick_[patchi].setSize(nInjectors, Zero);

        if (hasBaiGosman_)
        {
            nRebound_[patchi].setSize(nInjectors, Zero);
            massRebound_[patchi].setSize(nInjectors, Zero);
            nSplash_[patchi].setSize(nInjectors, Zero);
            massSplash_[patchi].setSize(nInjectors, Zero);
        }
    }
}


template<class CloudType>
Foam::LocalInteraction<CloudType>::LocalInteraction
(
    const LocalInteraction<CloudType>& pim
)
:
    PatchInteractionModel<CloudType>(pim),
    patchData_(pim.patchData_),
    nEscape_(pim.nEscape_),
    massEscape_(pim.massEscape_),
    nStick_(pim.nStick_),
    massStick_(pim.massStick_),
    nRebound_(pim.nRebound_),
    massRebound_(pim.massRebound_),
    nSplash_(pim.nSplash_),
    massSplash_(pim.massSplash_),
    writeFields_(pim.writeFields_),
    hasBaiGosman_(pim.hasBaiGosman_),
    beiGosman_(pim.beiGosman_, *this),
    injIdToIndex_(pim.injIdToIndex_),
    massEscapePtr_(nullptr),
    massStickPtr_(nullptr)
{}


// * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * * //

template<class CloudType>
bool Foam::LocalInteraction<CloudType>::hasBaiGosmanPatch() const
{
    forAll(patchData_, patchi)
    {
        if (patchData_[patchi].isBaiGosman())
        {
            return true;
        }
    }

    return false;
}


template<class CloudType>
void Foam::LocalInteraction<CloudType>::addEscapeCounters
(
    const label patchi,
    const label idx,
    const scalar mass,
    const polyPatch& pp,
    const label facei
)
{
    nEscape_[patchi][idx]++;
    massEscape_[patchi][idx] += mass;
    this->addToEscapedParcels(mass);

    if (writeFields_ && facei >= 0)
    {
        massEscape().boundaryFieldRef()[pp.index()][facei] += mass;
    }
}


template<class CloudType>
void Foam::LocalInteraction<CloudType>::addStickCounters
(
    const label patchi,
    const label idx,
    const scalar mass,
    const polyPatch& pp,
    const label facei,
    const bool incrementCount
)
{
    if (incrementCount)
    {
        nStick_[patchi][idx]++;
    }

    massStick_[patchi][idx] += mass;

    if (writeFields_ && facei >= 0)
    {
        massStick().boundaryFieldRef()[pp.index()][facei] += mass;
    }
}


template<class CloudType>
void Foam::LocalInteraction<CloudType>::addReboundCounters
(
    const label patchi,
    const label idx,
    const scalar mass
)
{
    nRebound_[patchi][idx]++;
    massRebound_[patchi][idx] += mass;
}


template<class CloudType>
void Foam::LocalInteraction<CloudType>::addSplashCounters
(
    const label patchi,
    const label idx,
    const scalar mass
)
{
    nSplash_[patchi][idx]++;
    massSplash_[patchi][idx] += mass;
}


// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

template<class CloudType>
Foam::volScalarField& Foam::LocalInteraction<CloudType>::massEscape()
{
    if (!massEscapePtr_)
    {
        const fvMesh& mesh = this->owner().mesh();

        massEscapePtr_.reset
        (
            new volScalarField
            (
                IOobject
                (
                    IOobject::scopedName(this->owner().name(), "massEscape"),
                    mesh.time().timeName(),
                    mesh.thisDb(),
                    IOobject::READ_IF_PRESENT,
                    IOobject::AUTO_WRITE,
                    IOobject::REGISTER
                ),
                mesh,
                dimensionedScalar(dimMass, Zero)
            )
        );
    }

    return *massEscapePtr_;
}


template<class CloudType>
Foam::volScalarField& Foam::LocalInteraction<CloudType>::massStick()
{
    if (!massStickPtr_)
    {
        const fvMesh& mesh = this->owner().mesh();

        massStickPtr_.reset
        (
            new volScalarField
            (
                IOobject
                (
                    IOobject::scopedName(this->owner().name(), "massStick"),
                    mesh.time().timeName(),
                    mesh.thisDb(),
                    IOobject::READ_IF_PRESENT,
                    IOobject::AUTO_WRITE,
                    IOobject::REGISTER
                ),
                mesh,
                dimensionedScalar(dimMass, Zero)
            )
        );
    }

    return *massStickPtr_;
}


template<class CloudType>
bool Foam::LocalInteraction<CloudType>::correct
(
    typename CloudType::parcelType& p,
    const polyPatch& pp,
    bool& keepParticle
)
{
    const label patchi = patchData_.applyToPatch(pp.index());

    if (patchi >= 0)
    {
        vector& U = p.U();

        // Location for storing the stats.
        const label idx =
        (
            injIdToIndex_.size()
          ? injIdToIndex_.lookup(p.typeId(), 0)
          : 0
        );

        if (patchData_[patchi].isBaiGosman())
        {
            return beiGosman_.correct
            (
                p,
                pp,
                patchi,
                idx,
                keepParticle,
                localInteractionModels::parcelHasThermo
                <
                    typename CloudType::parcelType
                >()
            );
        }

        typename PatchInteractionModel<CloudType>::interactionType it =
            this->wordToInteractionType
            (
                patchData_[patchi].interactionTypeName()
            );

        switch (it)
        {
            case PatchInteractionModel<CloudType>::itNone:
            {
                return false;
            }
            case PatchInteractionModel<CloudType>::itEscape:
            {
                keepParticle = false;
                p.active(false);
                U = Zero;

                const scalar dm = p.mass()*p.nParticle();

                if (hasBaiGosman_)
                {
                    addEscapeCounters
                    (
                        patchi,
                        idx,
                        dm,
                        pp,
                        pp.whichFace(p.face())
                    );
                }
                else
                {
                    nEscape_[patchi][idx]++;
                    massEscape_[patchi][idx] += dm;

                    if (writeFields_)
                    {
                        const label pI = pp.index();
                        const label fI = pp.whichFace(p.face());
                        massEscape().boundaryFieldRef()[pI][fI] += dm;
                    }
                }
                break;
            }
            case PatchInteractionModel<CloudType>::itStick:
            {
                keepParticle = true;
                p.active(false);
                U = Zero;

                const scalar dm = p.mass()*p.nParticle();

                if (hasBaiGosman_)
                {
                    addStickCounters
                    (
                        patchi,
                        idx,
                        dm,
                        pp,
                        pp.whichFace(p.face())
                    );
                }
                else
                {
                    nStick_[patchi][idx]++;
                    massStick_[patchi][idx] += dm;

                    if (writeFields_)
                    {
                        const label pI = pp.index();
                        const label fI = pp.whichFace(p.face());
                        massStick().boundaryFieldRef()[pI][fI] += dm;
                    }
                }
                break;
            }
            case PatchInteractionModel<CloudType>::itRebound:
            {
                keepParticle = true;
                p.active(true);

                vector nw;
                vector Up;

                this->owner().patchData(p, pp, nw, Up);

                // Calculate motion relative to patch velocity
                U -= Up;

                if (mag(Up) > 0 && mag(U) < this->Urmax())
                {
                    WarningInFunction
                        << "Particle U the same as patch "
                        << "    The particle has been removed" << nl << endl;

                    keepParticle = false;
                    p.active(false);
                    U = Zero;
                    break;
                }

                scalar Un = U & nw;
                vector Ut = U - Un*nw;

                if (Un > 0)
                {
                    U -= (1.0 + patchData_[patchi].e())*Un*nw;
                }

                U -= patchData_[patchi].mu()*Ut;

                // Return velocity to global space
                U += Up;

                if (hasBaiGosman_)
                {
                    addReboundCounters
                    (
                        patchi,
                        idx,
                        p.mass()*p.nParticle()
                    );
                }

                break;
            }
            default:
            {
                FatalErrorInFunction
                    << "Unknown interaction type "
                    << patchData_[patchi].interactionTypeName()
                    << "(" << it << ") for patch "
                    << patchData_[patchi].patchName()
                    << ". Valid selections are:" << this->interactionTypeNames_
                    << " and " << patchInteractionData::beiGosmanTypeName
                    << endl << abort(FatalError);
            }
        }

        return true;
    }

    return false;
}


template<class CloudType>
void Foam::LocalInteraction<CloudType>::info()
{
    PatchInteractionModel<CloudType>::info();

    if (Pstream::parRun())
    {
        const bool anyBaiGosman = returnReduceOr(hasBaiGosman_);
        const bool allBaiGosman = returnReduceAnd(hasBaiGosman_);

        if (anyBaiGosman != allBaiGosman)
        {
            FatalErrorInFunction
                << "Inconsistent BaiGosman configuration across processors."
                << exit(FatalError);
        }

        const label nPatches = patchData_.size();
        const label minPatches = returnReduce(nPatches, minOp<label>());
        const label maxPatches = returnReduce(nPatches, maxOp<label>());

        if (minPatches != maxPatches)
        {
            FatalErrorInFunction
                << "Inconsistent localInteraction patch counter dimensions "
                << "across processors: min = " << minPatches
                << ", max = " << maxPatches
                << exit(FatalError);
        }

        forAll(nEscape_, patchi)
        {
            const label nInjectors = nEscape_[patchi].size();
            const label minInjectors =
                returnReduce(nInjectors, minOp<label>());
            const label maxInjectors =
                returnReduce(nInjectors, maxOp<label>());

            if (minInjectors != maxInjectors)
            {
                FatalErrorInFunction
                    << "Inconsistent localInteraction injector counter "
                    << "dimensions across processors for patch entry "
                    << patchData_[patchi].patchName()
                    << ": min = " << minInjectors
                    << ", max = " << maxInjectors
                    << exit(FatalError);
            }
        }
    }

    // retrieve any stored data
    labelListList npe0(patchData_.size());
    scalarListList mpe0(patchData_.size());
    labelListList nps0(patchData_.size());
    scalarListList mps0(patchData_.size());
    labelListList nRebound0(patchData_.size());
    scalarListList massRebound0(patchData_.size());
    labelListList nSplash0(patchData_.size());
    scalarListList massSplash0(patchData_.size());

    forAll(patchData_, patchi)
    {
        label lsd = nEscape_[patchi].size();
        npe0[patchi].setSize(lsd, Zero);
        mpe0[patchi].setSize(lsd, Zero);
        nps0[patchi].setSize(lsd, Zero);
        mps0[patchi].setSize(lsd, Zero);

        if (hasBaiGosman_)
        {
            nRebound0[patchi].setSize(lsd, Zero);
            massRebound0[patchi].setSize(lsd, Zero);
            nSplash0[patchi].setSize(lsd, Zero);
            massSplash0[patchi].setSize(lsd, Zero);
        }
    }


    this->getModelProperty("nEscape", npe0);
    this->getModelProperty("massEscape", mpe0);
    this->getModelProperty("nStick", nps0);
    this->getModelProperty("massStick", mps0);

    if (hasBaiGosman_)
    {
        this->getModelProperty("nRebound", nRebound0);
        this->getModelProperty("massRebound", massRebound0);
        this->getModelProperty("nSplash", nSplash0);
        this->getModelProperty("massSplash", massSplash0);
    }

    // accumulate current data
    labelListList npe(nEscape_);
    forAll(npe, i)
    {
        Pstream::listGather(npe[i], sumOp<label>());
        npe[i] = npe[i] + npe0[i];
    }

    scalarListList mpe(massEscape_);
    forAll(mpe, i)
    {
        Pstream::listGather(mpe[i], sumOp<scalar>());
        mpe[i] = mpe[i] + mpe0[i];
    }

    labelListList nps(nStick_);
    forAll(nps, i)
    {
        Pstream::listGather(nps[i], sumOp<label>());
        nps[i] = nps[i] + nps0[i];
    }

    scalarListList mps(massStick_);
    forAll(nps, i)
    {
        Pstream::listGather(mps[i], sumOp<scalar>());
        mps[i] = mps[i] + mps0[i];
    }

    labelListList nRebound(nRebound_);
    scalarListList massRebound(massRebound_);
    labelListList nSplash(nSplash_);
    scalarListList massSplash(massSplash_);

    if (hasBaiGosman_)
    {
        forAll(nRebound, i)
        {
            Pstream::listGather(nRebound[i], sumOp<label>());
            nRebound[i] = nRebound[i] + nRebound0[i];
        }

        forAll(massRebound, i)
        {
            Pstream::listGather(massRebound[i], sumOp<scalar>());
            massRebound[i] = massRebound[i] + massRebound0[i];
        }

        forAll(nSplash, i)
        {
            Pstream::listGather(nSplash[i], sumOp<label>());
            nSplash[i] = nSplash[i] + nSplash0[i];
        }

        forAll(massSplash, i)
        {
            Pstream::listGather(massSplash[i], sumOp<scalar>());
            massSplash[i] = massSplash[i] + massSplash0[i];
        }
    }

    if (injIdToIndex_.size())
    {
        // Since injIdToIndex_ is a one-to-one mapping (starting at zero),
        // can simply invert it.
        labelList indexToInjector(injIdToIndex_.size());
        forAllConstIters(injIdToIndex_, iter)
        {
            indexToInjector[iter.val()] = iter.key();
        }

        forAll(patchData_, patchi)
        {
            forAll(mpe[patchi], indexi)
            {
                const word& patchName = patchData_[patchi].patchName();

                Log_<< "    Parcel fate: patch " <<  patchName
                    << " (number, mass)" << nl
                    << "      - escape  (injector " << indexToInjector[indexi]
                    << " )  = " << npe[patchi][indexi]
                    << ", " << mpe[patchi][indexi] << nl
                    << "      - stick   (injector " << indexToInjector[indexi]
                    << " )  = " << nps[patchi][indexi]
                    << ", " << mps[patchi][indexi] << nl;

                if (hasBaiGosman_)
                {
                    Log_<< "      - rebound (injector "
                        << indexToInjector[indexi]
                        << " )  = " << nRebound[patchi][indexi]
                        << ", " << massRebound[patchi][indexi] << nl
                        << "      - splash  (injector "
                        << indexToInjector[indexi]
                        << " )  = " << nSplash[patchi][indexi]
                        << ", " << massSplash[patchi][indexi] << nl;
                }
            }
        }
    }
    else
    {
        forAll(patchData_, patchi)
        {
            const word& patchName = patchData_[patchi].patchName();

            Log_<< "    Parcel fate: patch " << patchName
                << " (number, mass)" << nl
                << "      - escape                      = "
                << npe[patchi][0] << ", " << mpe[patchi][0] << nl
                << "      - stick                       = "
                << nps[patchi][0] << ", " << mps[patchi][0] << nl;

            if (hasBaiGosman_)
            {
                Log_<< "      - rebound                     = "
                    << nRebound[patchi][0] << ", "
                    << massRebound[patchi][0] << nl
                    << "      - splash                      = "
                    << nSplash[patchi][0] << ", "
                    << massSplash[patchi][0] << nl;
            }
        }
    }

    forAll(npe, patchi)
    {
        forAll(npe[patchi], injectori)
        {
            this->file()
                << tab << npe[patchi][injectori]
                << tab << mpe[patchi][injectori]
                << tab << nps[patchi][injectori]
                << tab << mps[patchi][injectori];

            if (hasBaiGosman_)
            {
                this->file()
                    << tab << nRebound[patchi][injectori]
                    << tab << massRebound[patchi][injectori]
                    << tab << nSplash[patchi][injectori]
                    << tab << massSplash[patchi][injectori];
            }
        }
    }

    this->file() << endl;

    if (this->writeTime())
    {
        this->setModelProperty("nEscape", npe);
        this->setModelProperty("massEscape", mpe);
        this->setModelProperty("nStick", nps);
        this->setModelProperty("massStick", mps);

        if (hasBaiGosman_)
        {
            this->setModelProperty("nRebound", nRebound);
            this->setModelProperty("massRebound", massRebound);
            this->setModelProperty("nSplash", nSplash);
            this->setModelProperty("massSplash", massSplash);
        }

        nEscape_ = Zero;
        massEscape_ = Zero;
        nStick_ = Zero;
        massStick_ = Zero;

        if (hasBaiGosman_)
        {
            nRebound_ = Zero;
            massRebound_ = Zero;
            nSplash_ = Zero;
            massSplash_ = Zero;
        }
    }
}


// ************************************************************************* //
