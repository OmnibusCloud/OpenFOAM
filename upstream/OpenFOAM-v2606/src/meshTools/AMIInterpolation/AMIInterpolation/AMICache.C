/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2025 OpenCFD Ltd.
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

#include "AMICache.H"
#include "AMIInterpolation.H"
#include "mathematicalConstants.H"

/* * * * * * * * * * * * * * * Static Member Data  * * * * * * * * * * * * * */

namespace Foam
{
    defineTypeNameAndDebug(AMICache, 0);
}

Foam::scalar Foam::AMICache::cacheThetaTolerance_ = 1e-8;

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

Foam::scalar Foam::AMICache::getRotationAngle(const point& globalPoint) const
{
    if (!coordSysPtr_)
    {
        FatalErrorInFunction
            << "No co-ordinate system available for theta evaluation"
            << abort(FatalError);
    }


    scalar theta = coordSysPtr_->localPosition(globalPoint).y();

    // Ensure 0 < theta < 2pi
    if (mag(theta) < cacheThetaTolerance_)
    {
        theta = 0;
    }
    else if (theta < 0)
    {
        theta += constant::mathematical::twoPi;
    }

    return theta;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::AMICache::AMICache(const dictionary& dict, const bool toSource)
:
    size_(dict.getOrDefault<label>("cacheSize", 0)),
    rotationAxis_(dict.getOrDefault<vector>("rotationAxis", Zero)),
    rotationCentre_(dict.getOrDefault<point>("rotationCentre", Zero)),
    nThetaStencilMax_(dict.getOrDefault<label>("nThetaStencilMax", 2)),
    forceCache_(dict.getOrDefault<bool>("forceCache", false)),
    restartTime_(dict.getOrDefault<scalar>("cacheRestartTime", GREAT)),
    complete_(false),
    toSource_(toSource),
    index0_(-1),
    index1_(-1),
    interpWeight_(0),
    coordSysPtr_(nullptr),
    theta_(),
    cachedSrcAddress_(),
    cachedSrcWeights_(),
    cachedSrcWeightsSum_(),
    cachedSrcMapPtr_(),
    cachedTgtAddress_(),
    cachedTgtWeights_(),
    cachedTgtWeightsSum_(),
    cachedTgtMapPtr_()
{
    if (size_ != 0)
    {
        theta_.resize(size_, GREAT);
        cachedSrcAddress_.resize(size_);
        cachedSrcWeights_.resize(size_);
        cachedSrcWeightsSum_.resize(size_);
        cachedSrcMapPtr_.resize(size_);
        cachedTgtAddress_.resize(size_);
        cachedTgtWeights_.resize(size_);
        cachedTgtWeightsSum_.resize(size_);
        cachedTgtMapPtr_.resize(size_);
    }
}


Foam::AMICache::AMICache(const bool toSource)
:
    size_(0),
    rotationAxis_(Zero),
    rotationCentre_(Zero),
    nThetaStencilMax_(2),
    forceCache_(false),
    restartTime_(GREAT),
    complete_(false),
    toSource_(toSource),
    index0_(-1),
    index1_(-1),
    interpWeight_(0),
    coordSysPtr_(nullptr),
    theta_(),
    cachedSrcAddress_(),
    cachedSrcWeights_(),
    cachedSrcWeightsSum_(),
    cachedSrcMapPtr_(),
    cachedTgtAddress_(),
    cachedTgtWeights_(),
    cachedTgtWeightsSum_(),
    cachedTgtMapPtr_()
{}


Foam::AMICache::AMICache(const AMICache& cache)
:
    size_(cache.size_),
    rotationAxis_(cache.rotationAxis_),
    rotationCentre_(cache.rotationCentre_),
    nThetaStencilMax_(cache.nThetaStencilMax_),
    forceCache_(cache.forceCache_),
    restartTime_(cache.restartTime_),
    complete_(cache.complete_),
    toSource_(cache.toSource_),
    index0_(cache.index0_),
    index1_(cache.index1_),
    interpWeight_(cache.interpWeight_),
    coordSysPtr_(nullptr),  // Need to clone as cylindricalCS
    theta_(cache.theta_),
    cachedSrcAddress_(cache.cachedSrcAddress_),
    cachedSrcWeights_(cache.cachedSrcWeights_),
    cachedSrcWeightsSum_(cache.cachedSrcWeightsSum_),
    cachedSrcMapPtr_(cache.cachedSrcMapPtr_.size()),  // Need to clone
    cachedTgtAddress_(cache.cachedTgtAddress_),
    cachedTgtWeights_(cache.cachedTgtWeights_),
    cachedTgtWeightsSum_(cache.cachedTgtWeightsSum_),
    cachedTgtMapPtr_(cache.cachedTgtMapPtr_.size())  // Need to clone
{
    if (cache.coordSysPtr_)
    {
        coordSysPtr_.reset(new coordSystem::cylindrical(cache.coordSysPtr_()));
    }

    forAll(cachedSrcMapPtr_, cachei)
    {
        cachedSrcMapPtr_[cachei].reset(cache.cachedSrcMapPtr_[cachei].clone());
    }

    forAll(cachedTgtMapPtr_, cachei)
    {
        cachedTgtMapPtr_[cachei].reset(cache.cachedTgtMapPtr_[cachei].clone());
    }
}


Foam::AMICache::AMICache
(
    const AMICache& cache,
    const AMIInterpolation& fineAMI,
    const labelList& sourceRestrictAddressing,
    const labelList& targetRestrictAddressing
)
:
    size_(cache.size_),
    rotationAxis_(cache.rotationAxis_),
    rotationCentre_(cache.rotationCentre_),
    nThetaStencilMax_(cache.nThetaStencilMax_),
    forceCache_(cache.forceCache_),
    restartTime_(cache.restartTime_),
    complete_(cache.complete_),
    toSource_(cache.toSource_),
    index0_(cache.index0_),
    index1_(cache.index1_),
    interpWeight_(cache.interpWeight_),
    coordSysPtr_(nullptr),
    theta_(cache.theta_),
    cachedSrcAddress_(cache.size_),
    cachedSrcWeights_(cache.size_),
    cachedSrcWeightsSum_(cache.size_),
    cachedSrcMapPtr_(cache.size_),
    cachedTgtAddress_(cache.size_),
    cachedTgtWeights_(cache.size_),
    cachedTgtWeightsSum_(cache.size_),
    cachedTgtMapPtr_(cache.size_)
{
    if (size_ > 0 && fineAMI.comm() != -1)
    {
        for (label cachei : {index0_, index1_})
        {
            if (cachei == -1) continue;

            scalarField dummySrcMagSf;
            labelListList srcAddress;
            scalarListList srcWeights;
            scalarField srcWeightsSum;
            autoPtr<mapDistribute> tgtMapPtr;

            AMIInterpolation::agglomerate
            (
                cache.cachedTgtMapPtr()[cachei],
                fineAMI.srcMagSf(),
                cache.cachedSrcAddress()[cachei],
                cache.cachedSrcWeights()[cachei],

                sourceRestrictAddressing,
                targetRestrictAddressing,

                dummySrcMagSf,
                srcAddress,
                srcWeights,
                srcWeightsSum,
                tgtMapPtr,
                fineAMI.comm()
            );

            scalarField dummyTgtMagSf;
            labelListList tgtAddress;
            scalarListList tgtWeights;
            scalarField tgtWeightsSum;
            autoPtr<mapDistribute> srcMapPtr;

            AMIInterpolation::agglomerate
            (
                cache.cachedSrcMapPtr()[cachei],
                fineAMI.tgtMagSf(),
                cache.cachedTgtAddress()[cachei],
                cache.cachedTgtWeights()[cachei],

                targetRestrictAddressing,
                sourceRestrictAddressing,

                dummyTgtMagSf,
                tgtAddress,
                tgtWeights,
                tgtWeightsSum,
                srcMapPtr,
                fineAMI.comm()
            );

            cachedSrcAddress_[cachei] = srcAddress;
            cachedSrcWeights_[cachei] = srcWeights;
            cachedSrcWeightsSum_[cachei] = srcWeightsSum;
            cachedSrcMapPtr_[cachei] = srcMapPtr.clone();

            cachedTgtAddress_[cachei] = tgtAddress;
            cachedTgtWeights_[cachei] = tgtWeights;
            cachedTgtWeightsSum_[cachei] = tgtWeightsSum;
            cachedTgtMapPtr_[cachei] = tgtMapPtr.clone();
        }
    }
}


Foam::AMICache::AMICache(Istream& is)
:
    size_(readLabel(is)),

    rotationAxis_(is),
    rotationCentre_(is),
    nThetaStencilMax_(readLabel(is)),
    forceCache_(readBool(is)),
    restartTime_(readScalar(is)),

    complete_(readBool(is)),
    toSource_(readBool(is)),

    index0_(-1),
    index1_(-1),
    interpWeight_(0),
    coordSysPtr_(nullptr),
    theta_(),
    cachedSrcAddress_(),
    cachedSrcWeights_(),
    cachedSrcWeightsSum_(),
    cachedSrcMapPtr_(),
    cachedTgtAddress_(),
    cachedTgtWeights_(),
    cachedTgtWeightsSum_(),
    cachedTgtMapPtr_()
{
    const bitSet goodMap(is);

    if (goodMap.size())
    {
        is >> index0_
           >> index1_
           >> interpWeight_
           >> theta_;

        const bool goodCoord(readBool(is));
        if (goodCoord)
        {
            coordSysPtr_.reset(new coordSystem::cylindrical(is));
        }

        is >> cachedSrcAddress_
           >> cachedSrcWeights_
           >> cachedSrcWeightsSum_;

        cachedSrcMapPtr_.setSize(goodMap.size());
        forAll(goodMap, cachei)
        {
            if (goodMap[cachei])
            {
                cachedSrcMapPtr_[cachei].reset(new mapDistribute(is));
            }
        }

        is >> cachedTgtAddress_
           >> cachedTgtWeights_
           >> cachedTgtWeightsSum_;

        cachedTgtMapPtr_.setSize(goodMap.size());
        forAll(goodMap, cachei)
        {
            if (goodMap[cachei])
            {
                cachedTgtMapPtr_[cachei].reset(new mapDistribute(is));
            }
        }
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::AMICache::clearCache()
{
    Info<< "-- clearing cache" << endl;

    complete_ = false;

    // Reset the interpolation indices and weight
    index0_ = -1;
    index1_ = -1;
    interpWeight_ = 0;

    // Clear the cached values
    // Note: preserving the parallel maps
    for (label cachei = 0; cachei < size_; ++cachei)
    {
        theta_[cachei] = GREAT;

        cachedSrcAddress_[cachei].clear();
        cachedSrcWeights_[cachei].clear();
        cachedSrcWeightsSum_[cachei].clear();
        cachedSrcMapPtr_[cachei].clear();

        cachedTgtAddress_[cachei].clear();
        cachedTgtWeights_[cachei].clear();
        cachedTgtWeightsSum_[cachei].clear();
        cachedTgtMapPtr_[cachei].clear();
    }
}


void Foam::AMICache::addToCache
(
    const AMIInterpolation& ami,
    const point& globalPoint
)
{
    DebugPout<< "-- addToCache" << endl;


    if (!active())
    {
        DebugInfo<< "-- addToCache - deactivated" << endl;
        return;
    }

    if (!coordSysPtr_)
    {
        DebugInfo
            << "Creating rotation co-ordinate system:"
            << " rotationCentre:" << rotationCentre_
            << " rotationAxis:" << rotationAxis_
            << " p:" << globalPoint
            << endl;

        coordSysPtr_.reset
        (
            new coordSystem::cylindrical(rotationCentre_, rotationAxis_)
        );
        DebugPout<< "Coord sys:" << coordSysPtr_() << endl;
    }

    // Check if cache is complete
    if (!complete_)
    {
        complete_ = true;
        forAll(theta_, bini)
        {
            if (!validIndex(bini))
            {
                complete_ = false;
                break;
            }
        }
    }

    if (!complete_)
    {
        const scalar theta = getRotationAngle(globalPoint);
        const label bini = thetaIndex(theta);

        DebugPout<< "  -- bini:" << bini << " for theta:" << theta << endl;

        // Check if already have entry for this bin
        if (!validIndex(bini))
        {
            DebugPout<< "  -- setting cache at index " << bini << endl;

            // New entry
            theta_[bini] = theta;

            cachedSrcAddress_[bini] = ami.srcAddress();
            cachedSrcWeights_[bini] = ami.srcWeights();
            cachedSrcWeightsSum_[bini] = ami.srcWeightsSum();

            if (ami.hasSrcMap())
            {
                cachedSrcMapPtr_[bini] = ami.srcMap().clone();
            }

            cachedTgtAddress_[bini] = ami.tgtAddress();
            cachedTgtWeights_[bini] = ami.tgtWeights();
            cachedTgtWeightsSum_[bini] = ami.tgtWeightsSum();

            if (ami.hasTgtMap())
            {
                cachedTgtMapPtr_[bini] = ami.tgtMap().clone();
            }
        }
    }
}


bool Foam::AMICache::restoreCache(const point& globalPoint, const Time& runTime)
{
    DebugPout<< "-- restoreCache" << endl;

    if (runTime.value() >= restartTime_)
    {
        clearCache();
        restartTime_ = GREAT;
        return false;
    }

    index0_ = -1;
    index1_ = -1;
    interpWeight_ = -1;

    if (!coordSysPtr_ || size_ == -1)
    {
        return false;
    }

    const scalar theta = getRotationAngle(globalPoint);
    const label bini = thetaIndex(theta);

    DebugPout<< "  -- bini:" << bini << " for theta:" << theta << endl;

    if (!validIndex(bini) && forceCache_)
    {
        DebugPout
            << "  -- no cache available at bini - forcing evaluation" << endl;
        return false;
    }

    // Maximum angle in radians for which to search for cached bins
    const scalar twoPi = constant::mathematical::twoPi;

    if
    (
        validIndex(bini)
     && (
            mag(theta - theta_[bini]) < cacheThetaTolerance_
         || mag(theta - twoPi - theta_[bini]) < cacheThetaTolerance_
        )
    )
    {
        // Hit cached value - no interpolation needed
        // index1_ = -1 indicates no interpolation
        index0_ = bini;
        index1_ = -1;
        interpWeight_ = 0;

        DebugInfo
            << "  -- t0:" << theta_[index0_] << " theta:" << theta
            << " i0:" << index0_ << " i1:" << index1_
            << " w:" << interpWeight_ << endl;
        return true;
    }
    else
    {
        // Find indices and values bracketing theta
        const label nBin = theta_.size();

        // Participating theta values and bin addresses
        // - Note we add wrap-around values at start and end
        DynamicList<scalar> thetap(nBin+2);
        DynamicList<label> binAddresses(nBin+2);

        // Initialise wrap-around values
        thetap.push_back(0);
        binAddresses.push_back(-1);

        // Add valid cached values
        // - Note if the complete_ flag is set we could skip the validIndex
        //   check or avoid the need to copy the values across
        forAll(theta_, thetai)
        {
            if (validIndex(thetai))
            {
                thetap.push_back(theta_[thetai]);
                binAddresses.push_back(thetai);
            }
        }

        // Check that we have enough data points for interpolation
        // - We added storage for lower wrap-around value, and we then need
        //   at least 2 additional values for the interpolation
        if (thetap.size() < 3)
        {
            DebugPout<< "  -- no cache available" << endl;
            return false;
        }

        // Set wrap-around values if we have sufficient data
        thetap[0] = thetap.last() - twoPi;
        binAddresses[0] = binAddresses.last();
        thetap.push_back(thetap[1] + twoPi);
        binAddresses.push_back(binAddresses[1]);

        // Find the lower and upper indices in the thetap list bracketing theta
        label loweri = labelMax;
        label upperi = labelMax;
        forAll(thetap, i)
        {
            if (thetap[i] <= theta)
            {
                loweri = i;
            }
            else
            {
                upperi = i;
                break;
            }
        }

        if (loweri == labelMax || upperi == labelMax)
        {
            DebugPout<< "  -- no cache available" << endl;
            return false;
        }

        // Ensure distances are valid
        if (upperi == loweri)
        {
            // Should not get here - go back to direct hit?
            DebugPout
                << "  -- no cache available: theta:" << theta
                << " lower:" << loweri << " upper:" << upperi << endl;
            return false;
        }

        // Check that bin distances are valid
        // - Note: bin addresses wrap around
        label lowerBin = binAddresses[loweri];
        label upperBin = binAddresses[upperi];
        if (bini > binAddresses[upperi])
        {
            upperBin += size_;
        }
        if (bini < binAddresses[loweri])
        {
            lowerBin -= size_;
        }

        if
        (
            (bini - lowerBin) > nThetaStencilMax_
         || (upperBin - bini) > nThetaStencilMax_
        )
        {
            DebugPout
                << "  -- no cache available within nThetaStencilMax_:"
                << "     bini:" << bini << " lower:" << lowerBin
                << " upper:" << upperBin << endl;
            return false;
        }

        index0_ = binAddresses[loweri];
        index1_ = binAddresses[upperi];
        interpWeight_ =
            (theta - thetap[loweri])/(thetap[upperi] - thetap[loweri]);

        DebugInfo
            << theta_.size()
            << "  -- t0:" << theta_[index0_] << " theta:" << theta
            << " t1:" << theta_[index1_]
            << " i0:" << index0_ << " i1:" << index1_
            << " w:" << interpWeight_ << endl;

        if (debug > 1)
        {
            DebugPout
                << "Cached values for index0" << nl
                << "  - src addressing:" << cachedSrcAddress_[index0_] << nl
                << "  - src weights:" << cachedSrcWeights_[index0_] << nl
                << "  - tgt addressing:" << cachedTgtAddress_[index0_] << nl
                << "  - tgt weights:" << cachedTgtWeights_[index0_] << nl
                << "Cached values for index1" << nl
                << "  - src addressing:" << cachedSrcAddress_[index1_] << nl
                << "  - src weights:" << cachedSrcWeights_[index1_] << nl
                << "  - tgt addressing:" << cachedTgtAddress_[index1_] << nl
                << "  - tgt weights:" << cachedTgtWeights_[index1_] << nl
                << endl;
        }


        return true;
    }

    // If we get here then no valid cache found within stencil
    DebugPout<< "  -- no cache available" << endl;
    return false;
}


void Foam::AMICache::write(Ostream& os) const
{
    if (size_ > 0)
    {
        os.writeEntry("cacheSize", size_);
        os.writeEntry("rotationAxis", rotationAxis_);
        os.writeEntry("rotationCentre", rotationCentre_);
        os.writeEntry("nThetaStencilMax", nThetaStencilMax_);
        os.writeEntry("forceCache", forceCache_);
        os.writeEntry("cacheRestartTime", restartTime_);
    }
}


bool Foam::AMICache::writeData(Ostream& os) const
{
    os  << token::SPACE<< size_
        << token::SPACE<< rotationAxis_
        << token::SPACE<< rotationCentre_
        << token::SPACE<< nThetaStencilMax_
        << token::SPACE<< forceCache_
        << token::SPACE<< restartTime_
        << token::SPACE<< complete_
        << token::SPACE<< toSource_;

    bitSet goodMap(cachedSrcMapPtr_.size());
    forAll(goodMap, cachei)
    {
        goodMap.set(cachei, cachedSrcMapPtr_[cachei].good());
    }
    os  << token::SPACE << goodMap;

    if (goodMap.size())
    {
        os  << token::SPACE << index0_
            << token::SPACE << index1_
            << token::SPACE << interpWeight_
            << token::SPACE << theta_;

        os  << token::SPACE << coordSysPtr_.good();

        if (coordSysPtr_.good())
        {
            os  << token::SPACE << coordSysPtr_();
        }

        os  << token::SPACE << cachedSrcAddress_
            << token::SPACE << cachedSrcWeights_
            << token::SPACE << cachedSrcWeightsSum_;

        for (const auto& index : goodMap)
        {
            os  << token::SPACE << cachedSrcMapPtr_[index]();
        }

        os  << token::SPACE << cachedTgtAddress_
            << token::SPACE << cachedTgtWeights_
            << token::SPACE << cachedTgtWeightsSum_;

        for (const auto& index : goodMap)
        {
            os  << token::SPACE << cachedTgtMapPtr_[index]();
        }
    }

    return true;
}


// ************************************************************************* //
