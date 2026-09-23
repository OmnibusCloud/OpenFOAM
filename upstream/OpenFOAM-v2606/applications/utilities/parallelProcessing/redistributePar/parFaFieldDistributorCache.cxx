/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2022-2026 OpenCFD Ltd.
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

#include "parFaFieldDistributorCache.H"

#include "areaFields.H"
#include "edgeFields.H"
#include "fieldsDistributor.H"
#include "faMeshDistributor.H"
#include "faMeshSubset.H"
#include "faMeshTools.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

template<class BoolListType>
void Foam::parFaFieldDistributorCache::readImpl
(
    const Time& baseRunTime,
    const fileName& proc0CaseName,
    const bool decompose,  // i.e. read from undecomposed case

    const BoolListType& areaMeshOnProc,
    const refPtr<fileOperation>& readHandler,
    const fileName& areaMeshInstance,
    faMesh& mesh
)
{
    auto& runTime = const_cast<Time&>(mesh.time());
    const bool oldProcCase = runTime.processorCase();

    autoPtr<faMeshSubset> subsetterPtr;

    // Missing an area mesh somewhere?
    if (!areaMeshOnProc.all())
    {
        const auto oldParRun = UPstream::parRun(false);
        const auto oldCache = fileOperation::cacheLevel(0);

        // A zero-sized mesh with boundaries.
        // This is used to create zero-sized fields.
        subsetterPtr.reset(new faMeshSubset(mesh, Foam::zero{}));
        auto& subMesh = subsetterPtr().subMesh();

        // Restore states
        fileOperation::cacheLevel(oldCache);
        UPstream::parRun(oldParRun);

        // Avoid conflicts...
        faMeshTools::forceDemandDriven(subMesh);
    }

    // Get original objects (before incrementing time!)
    if (UPstream::master() && decompose)
    {
        Info<< "Setting caseName to " << baseRunTime.caseName()
            << " to read finite-area IOobjects" << endl;
        runTime.caseName() = baseRunTime.caseName();
        runTime.processorCase(false);
    }

    IOobjectList objects;
    if (readHandler)
    {
        auto handler = readHandler.shallowClone();
        handler = fileOperation::fileHandler(handler);
        auto oldComm = UPstream::commWorld(fileHandler().comm());

        objects = IOobjectList(mesh.thisDb(), runTime.timeName());

        // Restore
        (void)UPstream::commWorld(oldComm);
        (void)fileOperation::fileHandler(handler);
    }

    if (UPstream::master() && decompose)
    {
        Info<< "Restoring caseName (finite-area IOobjects)" << endl;
        runTime.caseName() = proc0CaseName;
        runTime.processorCase(oldProcCase);
    }

    Info<< "From time " << runTime.timeName()
        << " mesh:" << mesh.thisDb().objectRelPath()
        << " have objects:" << objects.names() << endl;

    if (UPstream::master() && decompose)
    {
        runTime.caseName() = baseRunTime.caseName();
        runTime.processorCase(false);
    }

    // Field reading

    #undef  doFieldReading
    #define doFieldReading(Storage)                                   \
    fieldsDistributor::readFields                                     \
    (                                                                 \
        areaMeshOnProc, readHandler, mesh, subsetterPtr, objects,     \
        Storage,                                                      \
        true  /* (deregister field) */                                \
    );

    // areaFields
    doFieldReading(scalarAreaFields_);
    doFieldReading(vectorAreaFields_);
    doFieldReading(sphericalTensorAreaFields_);
    doFieldReading(symmTensorAreaFields_);
    doFieldReading(tensorAreaFields_);

    // edgeFields
    doFieldReading(scalarEdgeFields_);
    doFieldReading(vectorEdgeFields_);
    doFieldReading(tensorEdgeFields_);
    doFieldReading(sphericalTensorEdgeFields_);
    doFieldReading(symmTensorEdgeFields_);
    #undef doFieldReading


    // Done reading
    if (decompose)
    {
        runTime.caseName() = proc0CaseName;
        runTime.processorCase(oldProcCase);
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::parFaFieldDistributorCache::read
(
    const Time& baseRunTime,
    const fileName& proc0CaseName,
    const bool decompose,  // i.e. read from undecomposed case

    const bitSet& areaMeshOnProc,
    const refPtr<fileOperation>& readHandler,
    const fileName& areaMeshInstance,
    faMesh& mesh
)
{
    readImpl
    (
        baseRunTime,
        proc0CaseName,
        decompose,

        areaMeshOnProc,
        readHandler,
        areaMeshInstance,
        mesh
    );
}


void Foam::parFaFieldDistributorCache::read
(
    const Time& baseRunTime,
    const fileName& proc0CaseName,
    const bool decompose,  // i.e. read from undecomposed case

    const boolUList& areaMeshOnProc,
    const refPtr<fileOperation>& readHandler,
    const fileName& areaMeshInstance,
    faMesh& mesh
)
{
    readImpl
    (
        baseRunTime,
        proc0CaseName,
        decompose,

        areaMeshOnProc,
        readHandler,
        areaMeshInstance,
        mesh
    );
}


void Foam::parFaFieldDistributorCache::redistributeAndWrite
(
    const faMeshDistributor& distributor
)
{
    distributor.redistributeAndWrite(scalarAreaFields_);
    distributor.redistributeAndWrite(vectorAreaFields_);
    distributor.redistributeAndWrite(sphericalTensorAreaFields_);
    distributor.redistributeAndWrite(symmTensorAreaFields_);
    distributor.redistributeAndWrite(tensorAreaFields_);

    distributor.redistributeAndWrite(scalarEdgeFields_);
    distributor.redistributeAndWrite(vectorEdgeFields_);
    distributor.redistributeAndWrite(sphericalTensorEdgeFields_);
    distributor.redistributeAndWrite(symmTensorEdgeFields_);
    distributor.redistributeAndWrite(tensorEdgeFields_);
}


bool Foam::parFaFieldDistributorCache::empty() const
{
    #undef  checkOperation
    #define checkOperation(Type) \
    (Type##AreaFields_.empty() && Type##EdgeFields_.empty())

    return
    (
        checkOperation(scalar)
     && checkOperation(vector)
     && checkOperation(sphericalTensor)
     && checkOperation(symmTensor)
     && checkOperation(tensor)
    );
    #undef checkOperation
}


void Foam::parFaFieldDistributorCache::info(Ostream& os) const
{
    do
    {
        bool isEmpty = true;

        #undef  doLocalCode
        #define doLocalCode(Tag, Member)                                      \
        if (!Member.empty())                                                  \
        {                                                                     \
            isEmpty = false;                                                  \
            os << Tag << ": ";                                                \
            PtrListOps::names(Member).writeList(os) << endl;                  \
        }

        doLocalCode("area scalar", scalarAreaFields_);
        doLocalCode("area vector", vectorAreaFields_);
        doLocalCode("area sphTensor", sphericalTensorAreaFields_);
        doLocalCode("area symmTensor", symmTensorAreaFields_);
        doLocalCode("area tensor", tensorAreaFields_);

        doLocalCode("edge scalar", scalarEdgeFields_);
        doLocalCode("edge vector", vectorEdgeFields_);
        doLocalCode("edge sphTensor", sphericalTensorEdgeFields_);
        doLocalCode("edge symmTensor", symmTensorEdgeFields_);
        doLocalCode("edge tensor", tensorEdgeFields_);

        #undef doLocalCode
        if (isEmpty)
        {
            os << "empty" << endl;
        }
    }
    while (false);
}


// ************************************************************************* //
