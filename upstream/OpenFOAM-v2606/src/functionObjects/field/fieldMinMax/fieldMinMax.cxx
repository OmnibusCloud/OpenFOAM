/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2015-2020 OpenCFD Ltd.
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

#include "fieldMinMax.H"
#include "fieldTypes.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
    defineTypeNameAndDebug(fieldMinMax, 0);
    addToRunTimeSelectionTable(functionObject, fieldMinMax, dictionary);
}
}

const Foam::Enum
<
    Foam::functionObjects::fieldMinMax::modeType
>
Foam::functionObjects::fieldMinMax::modeTypeNames_
({
    { modeType::mdMag,  "magnitude" },
    { modeType::mdCmpt, "component" },
});


// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

void Foam::functionObjects::fieldMinMax::writeFileHeader(Ostream& os)
{
    if (!fieldSet_.updateSelection())
    {
        return;
    }

    if (writtenHeader_)
    {
        writeBreak(file());
    }
    else
    {
        writeHeader(os, "Field minima and maxima");
    }

    writeCommented(os, "Time");

    if (location_)
    {
        writeTabbed(os, "field");
        writeTabbed(os, "min");
        writeTabbed(os, "location(min)");

        if (UPstream::parRun())
        {
            writeTabbed(os, "processor");
        }

        writeTabbed(os, "max");
        writeTabbed(os, "location(max)");

        if (UPstream::parRun())
        {
            writeTabbed(os, "processor");
        }
    }
    else
    {
        for (const word& fieldName : fieldSet_.selectionNames())
        {
            writeTabbed(os, "min(" + fieldName + ')');
            writeTabbed(os, "max(" + fieldName + ')');
        }
    }

    os  << endl;

    writtenHeader_ = true;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::fieldMinMax::fieldMinMax
(
    const word& name,
    const Time& runTime,
    const dictionary& dict
)
:
    fvMeshFunctionObject(name, runTime, dict),
    writeFile(mesh_, name, typeName, dict),
    internal_(false),
    location_(true),
    mode_(modeType::mdMag),
    fieldSet_(mesh_)
{
    read(dict);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::functionObjects::fieldMinMax::read(const dictionary& dict)
{
    fvMeshFunctionObject::read(dict);
    writeFile::read(dict);

    internal_ = dict.getOrDefault("internal", false);
    location_ = dict.getOrDefault("location", true);

    mode_ = modeTypeNames_.getOrDefault("mode", dict, modeType::mdMag);

    fieldSet_.read(dict);

    return true;
}


bool Foam::functionObjects::fieldMinMax::execute()
{
    return true;
}


bool Foam::functionObjects::fieldMinMax::write()
{
    writeFileHeader(file());

    if (!location_) writeCurrentTime(file());

    Log << type() << " " << name() <<  " write:" << nl;

    label count = 0;

    for (const word& fieldName : fieldSet_.selectionNames())
    {
        bool ok = obr_.contains(fieldName) &&
        (
            calcMinMaxFields<scalar>(fieldName, modeType::mdCmpt)
         || calcMinMaxFields<vector>(fieldName, mode_)
         || calcMinMaxFields<sphericalTensor>(fieldName, mode_)
         || calcMinMaxFields<symmTensor>(fieldName, mode_)
         || calcMinMaxFields<tensor>(fieldName, mode_)
        );

        if (ok)
        {
            ++count;
        }
     }

    if (!location_) file() << endl;

    if (debug)
    {
        Log << " : nFields=" << count;
    }
    Log << endl;

    return true;
}


// ************************************************************************* //
