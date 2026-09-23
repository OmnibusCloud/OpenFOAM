/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2017-2026 OpenCFD Ltd.
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

Application
    profilingSummary

Group
    grpMiscUtilities

Description
    Collects information from profiling files in the processor
    sub-directories and summarizes the number of calls and time spent as
    max/avg/min values. If the values are identical for all processes,
    only a single value is written.

\*---------------------------------------------------------------------------*/

#include "Time.H"
#include "polyMesh.H"
#include "OSspecific.H"
#include "IFstream.H"
#include "OFstream.H"
#include "argList.H"
#include "profiling.H"
#include "stringOps.H"
#include "timeSelector.H"
#include "IOobjectList.H"
#include "functionObject.H"

using namespace Foam;

// The name of the sub-dictionary entry for profiling fileName:
static const word profilingFileName("profiling");

// The name of the sub-dictionary entry for profiling:
static const word blockNameProfiling("profiling");

// The name of the sub-dictionary entry for profiling and tags of entries
// that will be processed to determine (max,avg,min) values
const HashTable<wordList> processing
{
    { "memInfo", { "hwm", "rss", "size", "free" } },
    { "profiling", { "calls", "totalTime", "childTime", "maxMem" } },
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::addNote
    (
        "Collect profiling information from processor directories and"
        " summarize time spent and number of calls as (max avg min) values."
    );

    timeSelector::addOptions(true, true);  // constant(true), zero(true)
    argList::noParallel();
    argList::noFunctionObjects();  // Never use function objects

    // The utility should work without problems when profiling is active
    // (since we don't trigger it anywhere), but disable explicitly
    // to avoid confusing messages:
    profiling::disable();

    #include "setRootCase.H"
    #include "createTime.H"

    // Determine the processor count
    const label nProcs = fileHandler().nProcs(args.path());

    if (!nProcs)
    {
        FatalErrorInFunction
            << "No processor* directories found"
            << exit(FatalError);
    }


    // Create the processor databases
    PtrList<Time> databases(nProcs);

    forAll(databases, proci)
    {
        databases.set
        (
            proci,
            new Time
            (
                Time::controlDictName,
                args.rootPath(),
                args.caseName()/("processor" + Foam::name(proci)),
                false,   // enableFunctionObjects = false
                false    // enableLibs = false
            )
        );
    }

    // Use the times list from the master processor
    // and select a subset based on the command-line options
    instantList timeDirs = timeSelector::select
    (
        databases[0].times(),
        args
    );

    if (timeDirs.empty())
    {
        WarningInFunction
            << "No times selected" << nl << endl;
        return 1;
    }

    // ----------------------------------------------------------------------

    // Processor local profiling information
    List<dictionary> profiles(nProcs);

    // Loop over all times
    forAll(timeDirs, timei)
    {
        // Set time for global database
        runTime.setTime(timeDirs[timei], timei);

        Info<< "Time = " << runTime.timeName() << endl;

        // Name/location for the output summary
        const fileName outputName
        {
            functionObject::outputPrefix,
            "profiling",
            runTime.timeName(),
            profilingFileName
        };


        label nDict = 0;

        // Set time for all databases
        forAll(databases, proci)
        {
            auto& dict = profiles[proci];
            dict.clear();

            databases[proci].setTime(timeDirs[timei], timei);

            // Look for "uniform/profiling" in each processor directory
            IOobjectList objects
            (
                databases[proci].time(),
                databases[proci].timeName(),
                "uniform"
            );

            if (const auto* ioptr = objects.findObject(profilingFileName))
            {
                dict = IOdictionary::readContents(*ioptr);

                // Assumed to be good if it has 'profiling' sub-dict
                if (dict.findDict(blockNameProfiling) != nullptr)
                {
                    ++nDict;
                }
            }

            if (nDict < proci)
            {
                break;
            }
        }

        if (nDict != nProcs)
        {
            Info<< "Found " << nDict << "/" << nProcs
                << " profiling files. Skipping this time step" << nl << endl;
            continue;
        }


        // Information seems to be there for all processors
        // can do a summary

        IOdictionary summary
        (
            IOobject
            (
                runTime.path()/outputName,
                runTime,
                IOobjectOption::NO_READ,
                IOobjectOption::NO_WRITE,
                IOobjectOption::NO_REGISTER,
                true   // global-like
            )
        );

        summary.note() =
        (
            "summarized (max avg min) values from "
          + Foam::name(nProcs) + " processors"
        );

        summary.set("memInfo", dictionary());
        summary.set("profiling", dictionary());

        // Accumulator for each tag
        HashTable<DynamicList<scalar>> stats;

        // Use first (processor0) to decide what others likely also have
        for (const entry& mainEntry : profiles.front())
        {
            // level1: eg, profiling {} or memInfo {}
            const word& level1Name = mainEntry.keyword();

            const wordList& tags =
                processing.lookup(level1Name, wordList::null());

            if
            (
                tags.empty()
             || (!mainEntry.isDict())
             || mainEntry.dict().empty()
            )
            {
                continue;  // Only process known types
            }
            const auto& level1Dict = mainEntry.dict();


            // We need to handle sub-dicts with other dicts
            //     Eg, trigger0 { .. } trigger1 { .. }
            //
            // and ones with primitives
            //     Eg, size xx; free yy;

            // Decide based on the first entry:

            // level2: eg, profiling { trigger0 { } }
            // or simply itself if it only contains primitives

            wordList level2Names;

            const bool hasDictEntries =
            (
                level1Dict.first()->isDict()
            );

            if (hasDictEntries)
            {
                level2Names = level1Dict.sortedToc(stringOps::natural_sort());
            }
            else
            {
                level2Names.resize(1, level1Name);
            }

            dictionary& outputDict = summary.subDictOrAdd(level1Name);

            for (const word& level2Name : level2Names)
            {
                // Presize everything
                stats.clear();
                for (const word& tag : tags)
                {
                    stats(tag).reserve(nProcs);
                }

                label nEntry = 0;

                for (const dictionary& procDict : profiles)
                {
                    const auto* inDictPtr = procDict.findDict(level1Name);

                    if (inDictPtr && hasDictEntries)
                    {
                        // Descend to the next level as required
                        inDictPtr = inDictPtr->findDict(level2Name);
                    }

                    if (!inDictPtr)
                    {
                        break;
                    }

                    ++nEntry;

                    for (const word& tag : tags)
                    {
                        if
                        (
                            scalar val;
                            inDictPtr->readIfPresent(tag, val, keyType::LITERAL)
                        )
                        {
                            stats(tag).push_back(val);
                        }
                    }
                }

                if (nEntry != nProcs)
                {
                    continue;
                }

                dictionary* outDictPtr = nullptr;

                // Make a full copy of this entry prior to editing it
                if (hasDictEntries)
                {
                    outputDict.add(level2Name, level1Dict.subDict(level2Name));
                    outDictPtr = outputDict.findDict(level2Name);
                }
                else
                {
                    // Merge into existing (empty) dictionary
                    summary.add(level1Name, level1Dict, true);
                    outDictPtr = &outputDict;
                }
                auto& outSubDict = *outDictPtr;

                // Remove trailing 'processor0' from any descriptions
                // (looks nicer)
                {
                    const word key("description");

                    if (string val; outSubDict.readIfPresent(key, val))
                    {
                        if (val.removeEnd("processor0"))
                        {
                            outSubDict.set(key, val);
                        }
                    }
                }

                // Process each tag (calls, time etc)
                for (const word& tag : tags)
                {
                    const auto& values = stats(tag);

                    if (values.size() == nProcs)
                    {
                        MinMax<scalar> limits(values);
                        scalar avg = Foam::sum(values) / nProcs;

                        if (limits.min() < limits.max())
                        {
                            outSubDict.set
                            (
                                tag,
                                FixedList<scalar, 3>
                                {
                                    limits.max(), avg, limits.min()
                                }
                            );
                        }
                    }
                }
            }
        }

        // Extract high-water mark (or peak) per-host information
        {
            // Per-host accumulator
            HashTable<DynamicList<scalar>> hwm_stats;

            for (const dictionary& procDict : profiles)
            {
                int64_t mem_hwm(-1);

                if (const auto* dictptr = procDict.findDict("memInfo"))
                {
                    if (!dictptr->readIfPresent("hwm", mem_hwm))
                    {
                        dictptr->readIfPresent("peak", mem_hwm);
                    }
                }

                if (mem_hwm < 0)
                {
                    continue;
                }

                string host;
                if (!procDict.readIfPresent("host", host))
                {
                    if (const auto* dictptr = procDict.findDict("sysInfo"))
                    {
                        dictptr->readIfPresent("host", host);
                    }
                }

                if (!host.empty())
                {
                    hwm_stats(host).push_back(mem_hwm);
                }
            }

            dictionary& outputDict =
                summary.subDictOrAdd("memInfo").subDictOrAdd("hwm_total");

            for (const auto& iter : hwm_stats.csorted())
            {
                scalar total = Foam::sum(iter.val());
                outputDict.add(word(iter.key()), total);
            }
        }

        // Now write the summary
        {
            Foam::mkDir(summary.path());

            OFstream os(summary.objectPath());

            summary.writeHeader(os);
            summary.writeData(os);
            IOobject::writeEndDivider(os);

            Info<< "Wrote to " << outputName << nl << endl;
        }
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
