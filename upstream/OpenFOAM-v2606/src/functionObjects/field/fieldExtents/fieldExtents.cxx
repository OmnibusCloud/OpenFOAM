/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2018-2023 OpenCFD Ltd.
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

#include "fieldExtents.H"
#include "emptyPolyPatch.H"
#include "processorPolyPatch.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
    defineTypeNameAndDebug(fieldExtents, 0);
    addToRunTimeSelectionTable(functionObject, fieldExtents, dictionary);
}
}


// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

void Foam::functionObjects::fieldExtents::writeFileHeader(Ostream& os)
{
    if (!fieldSet_.updateSelection())
    {
        return;
    }

    if (writtenHeader_)
    {
        writeBreak(os);
    }
    else
    {
        writeHeader(os, "Field extents");
        writeHeaderValue(os, "Reference position", C0_);
        writeHeaderValue(os, "Threshold", threshold_);
    }

    writeCommented(os, "Time");

    for (const word& fieldName : fieldSet_.selectionNames())
    {
        if (internalField_)
        {
            writeTabbed(os, fieldName + "_internal");
        }
        for (const label patchi : patchIDs_)
        {
            const word& patchName = mesh_.boundaryMesh()[patchi].name();
            writeTabbed(os, fieldName + "_" + patchName);
        }
    }

    os  << endl;

    writtenHeader_ = true;
}


// * * * * * * * * * * * * * * * Implementation * * * * * * * * * * * * * * * //

#include "fieldExtents_impl.cxx"


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::fieldExtents::fieldExtents
(
    const word& name,
    const Time& runTime,
    const dictionary& dict
)
:
    fvMeshFunctionObject(name, runTime, dict),
    writeFile(mesh_, name, typeName, dict),
    internalField_(true),
    threshold_(0),
    C0_(point::zero),
    fieldSet_(mesh_)
{
    read(dict);

    // Note: delay creating the output file header to handle field names
    // specified using regular expressions
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::functionObjects::fieldExtents::read(const dictionary& dict)
{
    const polyBoundaryMesh& pbm = mesh_.boundaryMesh();

    if (fvMeshFunctionObject::read(dict) && writeFile::read(dict))
    {
        dict.readIfPresent<bool>("internalField", internalField_);

        threshold_ = dict.get<scalar>("threshold");

        dict.readIfPresent<vector>("referencePosition", C0_);

        const label maxPatches = pbm.nNonProcessor();

        if (wordRes patchNames; dict.readIfPresent("patches", patchNames))
        {
            patchIDs_ = pbm.indices(patchNames);

            label nPatches = 0;

            // Remove any empty or processor patches
            forAll(patchIDs_, i)
            {
                const label patchi = patchIDs_[i];

                const polyPatch& pp = pbm[patchi];

                if
                (
                    patchi < maxPatches
                 && !isA<emptyPolyPatch>(pp)
                 && !isA<processorPolyPatch>(pp)
                )
                {
                    patchIDs_[nPatches] = patchi;
                    ++nPatches;
                }
            }

            patchIDs_.resize(nPatches);
        }
        else
        {
            patchIDs_.resize_nocopy(maxPatches);

            label nPatches = 0;

            // All non-empty, non-processor patches
            for (label patchi = 0; patchi < maxPatches; ++patchi)
            {
                const polyPatch& pp = pbm[patchi];

                if
                (
                    !isA<emptyPolyPatch>(pp)
                 && !isA<processorPolyPatch>(pp)
                )
                {
                    patchIDs_[nPatches] = patchi;
                    ++nPatches;
                }
            }

            patchIDs_.resize(nPatches);
        }

        if (!internalField_ && patchIDs_.empty())
        {
            IOWarningInFunction(dict)
                << "No internal field or patches selected - no field extent "
                << "information will be generated" << endl;
        }

        fieldSet_.read(dict);

        return true;
    }

    return false;
}


bool Foam::functionObjects::fieldExtents::execute()
{
    return true;
}


bool Foam::functionObjects::fieldExtents::write()
{
    writeFileHeader(file());

    Log << type() << " " << name() <<  " write:" << nl;

    label count = 0;

    for (const word& fieldName : fieldSet_.selectionNames())
    {
        bool ok = obr_.contains(fieldName) &&
        (
            calcFieldExtents<scalar>(fieldName)
         || calcFieldExtents<vector>(fieldName)
         || calcFieldExtents<sphericalTensor>(fieldName)
         || calcFieldExtents<symmTensor>(fieldName)
         || calcFieldExtents<tensor>(fieldName)
        );

        if (ok)
        {
            ++count;
        }
    }

    if (debug)
    {
        Log << " : nFields=" << count;
    }
    Log << endl;

    return true;
}


// ************************************************************************* //
