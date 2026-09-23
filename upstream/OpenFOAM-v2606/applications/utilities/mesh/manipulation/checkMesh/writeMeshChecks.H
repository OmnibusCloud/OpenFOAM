enum class writeChecksFormatType
{
    none,
    dictionary,
    JSON
};
const Enum<writeChecksFormatType> writeChecksFormatTypeNames
{
    { writeChecksFormatType::none, "none" },
    { writeChecksFormatType::dictionary, "dictionary" },
    { writeChecksFormatType::JSON, "json" },
};

writeChecksFormatType writeChecksFormat(writeChecksFormatType::none);

auto writeMeshChecks = [&](const fvMesh& mesh, const writeChecksFormatType fmt)
{
    // Early exit if 'none' option is set
    if (fmt == writeChecksFormatType::none)
    {
        return;
    }

    auto& dataRef = mesh.dataRef();

    // Add base count metrics
    dataRef.setMeshMetric
    (
        "geometry", "cellCount", mesh.globalData().nTotalCells()
    );
    dataRef.setMeshMetric
    (
        "geometry", "faceCount", mesh.globalData().nTotalFaces()
    );
    dataRef.setMeshMetric
    (
        "geometry", "pointCount", mesh.globalData().nTotalPoints()
    );
    dataRef.setMeshMetric
    (
        "geometry", "patchCount", mesh.boundaryMesh().nNonProcessor()
    );

    // Summarise errors
    {
        DynamicList<Tuple2<word, label>> errorCounts;
        DynamicList<Tuple2<word, label>> warningCounts;
        forAllConstIters(mesh.data().meshDict(), iter)
        {
            if (iter().isDict())
            {
                const dictionary& metricDict = iter().dict();

                auto* warnPtr = metricDict.findEntry("warningCount");
                if (warnPtr)
                {
                    if (label nWarn(readLabel(warnPtr->stream())); nWarn > 0)
                    {
                        warningCounts.emplace_back(iter().keyword(), nWarn);
                    }
                }

                auto* errPtr = metricDict.findEntry("errorCount");
                if (errPtr)
                {
                    if (label nErr(readLabel(errPtr->stream())); nErr > 0)
                    {
                        errorCounts.emplace_back(iter().keyword(), nErr);
                    }
                }
            }
        }

        if (warningCounts.size())
        {
            dataRef.setMeshMetric("summary", "warningCounts", warningCounts);
        }

        if (errorCounts.size())
        {
            dataRef.setMeshMetric("summary", "state", "fail");
            dataRef.setMeshMetric("summary", "errorCounts", errorCounts);
        }
        else
        {
            dataRef.setMeshMetric("summary", "state", "pass");
        }
    }

    if (UPstream::master())
    {
        fileName path(mesh.time().globalPath()/"checkMesh");
        if (mesh.name() != polyMesh::defaultRegion)
        {
            path += "_" + mesh.name();
        }

        OFstream os(path.ext(writeChecksFormatTypeNames[fmt]));

        Info<< "Writing mesh data to " << os.name() << nl << endl;

        switch (fmt)
        {
            case writeChecksFormatType::dictionary:
            {
                IOdictionary data
                (
                    IOobject
                    (
                        os.name(),
                        mesh,
                        IOobject::NO_READ
                    )
                );

                data.writeHeader(os);

                mesh.data().meshDict().write(os, false);

                IOobject::writeEndDivider(os);

                break;
            }
            case writeChecksFormatType::JSON:
            {
                JSONformatter json(os);
                json.writeDict(mesh.data().meshDict());

                break;
            }
            default:
            {
                // Do nothing
            }
        }
    }
};
