/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2013 OpenFOAM Foundation
    Copyright (C) 2016-2025 OpenCFD Ltd.
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

#include "TimePaths.H"
#include "argList.H"
#include "fileOperation.H"
#include "IOstreams.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

bool Foam::TimePaths::detectProcessorCase()
{
    if (processorCase_)
    {
        return processorCase_;
    }

    // Look for "processor", but should really check for following digits too
    const auto sep = globalCaseName_.rfind('/');
    const auto pos = globalCaseName_.find
    (
        "processor",
        (sep == string::npos ? 0 : sep)
    );

    if (pos == 0)
    {
        globalCaseName_ = ".";
        processorCase_  = true;
    }
    else if (pos != string::npos && sep != string::npos && sep == pos-1)
    {
        globalCaseName_.resize(sep);
        processorCase_  = true;
    }

    return processorCase_;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::TimePaths::TimePaths
(
    const bool processorCase,
    const fileName& rootPath,
    const bool distributed,
    const fileName& globalCaseName,
    const fileName& caseName,
    const word& systemDirName,
    const word& constantDirName
)
:
    processorCase_(processorCase),
    distributed_(distributed),
    rootPath_(rootPath),
    globalCaseName_(globalCaseName),
    caseName_(caseName),
    system_(systemDirName),
    constant_(constantDirName)
{
    // Find out from case name whether it is a processor directory
    // and set processorCase flag so file searching goes up one level.
    detectProcessorCase();
}


Foam::TimePaths::TimePaths
(
    const argList& args,
    const word& systemDirName,
    const word& constantDirName
)
:
    TimePaths
    (
        args.runControl().parRun(),  // processorCase
        args.rootPath(),
        args.runControl().distributed(),
        args.globalCaseName(),
        args.caseName(),
        systemDirName,
        constantDirName
    )
{}


// * * * * * * * * * * * * * Static Member Functions * * * * * * * * * * * * //

Foam::instantList Foam::TimePaths::findTimes
(
    const fileName& directory,
    const word& constantDirName
)
{
    return fileHandler().findTimes(directory, constantDirName);
}


Foam::label Foam::TimePaths::findClosestTimeIndex
(
    const UList<instant>& timeDirs,
    const scalar t,
    const word& constantDirName
)
{
    const label nTimes = timeDirs.size();

    label nearestIndex = -1;
    scalar deltaT = GREAT;

    for (label timei=0; timei < nTimes; ++timei)
    {
        if (timeDirs[timei].name() == constantDirName) continue;

        const scalar diff = Foam::mag(timeDirs[timei].value() - t);
        if (diff < deltaT)
        {
            deltaT = diff;
            nearestIndex = timei;
        }
    }

    return nearestIndex;
}


Foam::instant Foam::TimePaths::findClosestTime
(
    const UList<instant>& timeDirs,
    const scalar t,
    const word& constantDirName
)
{
    const label nTimes = timeDirs.size();

    if (nTimes == 0)
    {
        // Cannot really fail at this point, but for some safety...
        return instant(0, constantDirName);
    }
    else if (nTimes == 1)
    {
        // Only one time (likely "constant") so return it
        return timeDirs[0];
    }
    else if (t < timeDirs[1].value())
    {
        return timeDirs[1];
    }
    else if (t > timeDirs[nTimes-1].value())
    {
        return timeDirs[nTimes-1];
    }

    label nearestIndex = 0;  // Failsafe value
    scalar deltaT = GREAT;

    for (label timei=1; timei < nTimes; ++timei)
    {
        const scalar diff = Foam::mag(timeDirs[timei].value() - t);
        if (diff < deltaT)
        {
            deltaT = diff;
            nearestIndex = timei;
        }
    }

    return timeDirs[nearestIndex];
}


Foam::word Foam::TimePaths::findInstancePath
(
    const UList<instant>& timeDirs,
    const instant& t
)
{
    // Note:
    // - timeDirs will include constant (with value 0) as first element.
    //   For backwards compatibility make sure to find 0 in preference
    //   to constant.
    // - list is sorted so could use binary search

    forAllReverse(timeDirs, i)
    {
        if (t.equal(timeDirs[i].value()))
        {
            return timeDirs[i].name();
        }
    }

    return word();
}


Foam::word Foam::TimePaths::findInstancePath
(
    const fileName& directory,
    const instant& t,
    const word& constantDirName
)
{
    // NB: uses fileHandler()
    instantList timeDirs = findTimes(directory, constantDirName);
    return findInstancePath(timeDirs, t);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::instantList Foam::TimePaths::times() const
{
    // NB: uses fileHandler()
    return findTimes(path(), constant());
}


Foam::instantList Foam::TimePaths::times(IOobjectOption::Layout layout) const
{
    // NB: uses fileHandler()
    return findTimes(path(layout), constant());
}


Foam::instant Foam::TimePaths::findClosestTime(const scalar t) const
{
    // NB: uses fileHandler()
    return findClosestTime
    (
        findTimes(path(), constant()),
        t,
        constant()
    );
}


Foam::instant Foam::TimePaths::findClosestTime
(
    IOobjectOption::Layout layout,
    const scalar t
) const
{
    // NB: uses fileHandler()
    return findClosestTime
    (
        findTimes(path(layout), constant()),
        t,
        constant()
    );
}


Foam::word Foam::TimePaths::findInstancePath(const instant& t) const
{
    return findInstancePath(path(), t);
}


Foam::word Foam::TimePaths::findInstancePath
(
    IOobjectOption::Layout layout,
    const instant& t
) const
{
    return findInstancePath(path(layout), t);
}


// ************************************************************************* //
