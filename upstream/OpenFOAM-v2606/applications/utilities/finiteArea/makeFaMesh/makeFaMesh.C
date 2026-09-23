/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2017 Wikki Ltd
    Copyright (C) 2021-2026 OpenCFD Ltd.
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
    makeFaMesh

Description
    A mesh generator for finiteArea mesh.
    When called in parallel, it will also try to act like decomposePar,
    create procAddressing and decompose serial finite-area fields.

Original Authors
    Zeljko Tukovic, FAMENA
    Hrvoje Jasak, Wikki Ltd.

\*---------------------------------------------------------------------------*/

#include "Time.H"
#include "argList.H"
#include "faMesh.H"
#include "IOdictionary.H"
#include "IOobjectList.H"
#include "areaFields.H"
#include "edgeFields.H"
#include "faFieldDecomposer.H"
#include "faMeshTools.H"
#include "faMeshReconstructor.H"
#include "faMeshSubset.H"
#include "PtrListOps.H"
#include "foamVtkIndPatchWriter.H"
#include "foamVtkLineWriter.H"
#include "foamVtkVertexWriter.H"
#include "regionProperties.H"
#include "syncTools.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::addNote
    (
        "A mesh generator for finiteArea mesh"
    );
    argList::addOption
    (
        "empty-patch",
        "name",
        "Specify name for a default empty patch",
        false  // An advanced option, but not enough to worry about that
    );
    argList::addOption("dict", "file", "Alternative faMeshDefinition");

    argList::addDryRunOption
    (
        "Create but do not write"
    );
    argList::addBoolOption
    (
        "no-decompose",
        "Suppress procAddressing creation and field decomposition"
        " (parallel)"
    );
    argList::addBoolOption
    (
        "no-fields",
        "Suppress field decomposition"
        " (parallel)"
    );
    argList::addBoolOption
    (
        "write-vtk",
        "Write mesh as a vtp (vtk) file for display or debugging"
    );
    argList::addBoolOption
    (
        "write-edges-vtk",
        "Write mesh edges as vtp (vtk) files (one per processor)",
        true  // advanced option (debugging only)
    );

    argList::addBoolOption
    (
        "disable-edge-encoding",
        "Emit edgeProcAddressing without encoding edge flips, "
        "as per 2512 and earlier [special use]",
        true  // Advanced option
    );

    #include "addRegionOption.H"
    #include "addAllFaRegionOptions.H"

    #include "setRootCase.H"
    #include "createTime.H"
    #include "createNamedPolyMesh.H"

    // Handle area region selections
    #include "getAllFaRegionOptions.H"

    const bool doDecompose = !args.found("no-decompose");
    const bool doDecompFields = !args.found("no-fields");

    // Special use - emit old (2512 and earlier) edgeProcAddressing format
    // without encoded edge flips.
    if (args.found("disable-edge-encoding"))
    {
        faMeshReconstructor::allowEdgeEncoding(false);
    }

    if (!doDecompose)
    {
        Info<< "Skip decompose of finiteArea mesh/fields" << nl;
    }
    else if (!doDecompFields)
    {
        Info<< "Skip decompose of finiteArea fields" << nl;
    }

    // ------------------------------------------------------------------------

    for (const word& areaRegionName : areaRegionNames)
    {
        // Reading faMeshDefinition dictionary
        #include "findMeshDefinitionDict.H"

        if (!faMeshDefinitionPtr)
        {
            if (args.dryRun())
            {
                break;
            }
            else
            {
                FatalErrorInFunction
                    << "Did not find area-mesh definition"<< nl
                    << exit(FatalError);
            }
        }

        auto& meshDefDict = (*faMeshDefinitionPtr);


        // Inject/overwrite name for optional 'empty' patch
        if (word name; args.readIfPresent("empty-patch", name))
        {
            meshDefDict.add("emptyPatch", name, true);
        }

        // Preliminary checks
        #include "checkPatchTopology.H"

        Info<< "Create area-mesh";
        if (!polyMesh::regionName(areaRegionName).empty())
        {
            Info<< " [" << areaRegionName << "]";
        }
        Info<< " with polyMesh at time = " << runTime.timeName() << nl;

        // Create
        faMesh aMesh(areaRegionName, mesh, meshDefDict);

        // Mesh information (less verbose)
        faMeshTools::printMeshChecks(aMesh, 0);

        if (args.found("write-vtk") || args.found("write-edges-vtk"))
        {
            #include "faMeshWriteVTK.H"
        }

        if (args.dryRun())
        {
            Info<< "\ndry-run: not writing mesh or decomposing fields\n" << nl;
        }
        else
        {
            // More precision (for points data)
            IOstream::minPrecision(10);

            Info<< nl << "Write finite area mesh";
            if (const word& name = aMesh.regionName(); !name.empty())
            {
                Info<< " [" << name << "]";
            }
            Info<< nl;

            aMesh.write();

            Info<< endl;
            #include "decomposeFaFields.H"
        }
    }

    Info<< "\nEnd\n" << endl;

    return 0;
}

// ************************************************************************* //
