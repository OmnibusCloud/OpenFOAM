/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2015-2022 OpenCFD Ltd.
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
#include "volFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class T>
Foam::labelPair Foam::functionObjects::fieldMinMax::findMinMax_mag
(
    const UList<T>& input,
    label start
)
{
    const label len = input.size();

    if (start < 0 || start >= len)
    {
        return labelPair(-1, -1);
    }

    label minIdx = start;
    label maxIdx = start;

    scalar minVal = Foam::mag(input[start]);
    scalar maxVal = minVal;

    for (label i = start+1; i < len; ++i)
    {
        const scalar val = Foam::mag(input[i]);
        if (val < minVal)
        {
            minIdx = i;
            minVal = val;
        }
        if (maxVal < val)
        {
            maxIdx = i;
            maxVal = val;
        }
    }

    return labelPair(minIdx, maxIdx);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
void Foam::functionObjects::fieldMinMax::output
(
    const word& outputName,
    const label minCell,
    const label maxCell,
    const vector& minPosition,
    const vector& maxPosition,
    const label minProci,
    const label maxProci,
    const Type& minValue,
    const Type& maxValue
)
{
    OFstream& file = this->file();

    if (location_)
    {
        writeCurrentTime(file);

        writeTabbed(file, outputName);

        file<< token::TAB << minValue
            << token::TAB << minPosition;

        if (UPstream::parRun())
        {
            file<< token::TAB << minProci;
        }

        file<< token::TAB << maxValue
            << token::TAB << maxPosition;

        if (UPstream::parRun())
        {
            file<< token::TAB << maxProci;
        }

        file<< endl;

        Log << "    min(" << outputName << ") = " << minValue
            << " in cell " << minCell
            << " at location " << minPosition;

        if (UPstream::parRun())
        {
            Log << " on processor " << minProci;
        }

        Log << nl << "    max(" << outputName << ") = " << maxValue
            << " in cell " << maxCell
            << " at location " << maxPosition;

        if (UPstream::parRun())
        {
            Log << " on processor " << maxProci;
        }
    }
    else
    {
        file<< token::TAB << minValue << token::TAB << maxValue;

        Log << "    min/max(" << outputName << ") = "
            << minValue << ' ' << maxValue;
    }

    Log << endl;

    // Write state/results information
    word nameStr('(' + outputName + ')');
    this->setResult("min" + nameStr, minValue);
    this->setResult("min" + nameStr + "_cell", minCell);
    this->setResult("min" + nameStr + "_position", minPosition);
    this->setResult("min" + nameStr + "_processor", minProci);
    this->setResult("max" + nameStr, maxValue);
    this->setResult("max" + nameStr + "_cell", maxCell);
    this->setResult("max" + nameStr + "_position", maxPosition);
    this->setResult("max" + nameStr + "_processor", maxProci);
}


template<class Type, Foam::functionObjects::fieldMinMax::modeType Mode>
void Foam::functionObjects::fieldMinMax::calcMinMaxFieldType
(
    const GeometricField<Type, fvPatchField, volMesh>& field,
    const word& outputFieldName
)
{
    const auto myProci = UPstream::myProcNo();
    const auto numProc = UPstream::nProcs();

    // NOTE: the code here is largely identical to fieldStatistics
    // For modeType::mdMag, we compute on magnitudes (scalar values)

    // Magnitude mode: scalar; Component mode: Type
    // (same as std::conditional_t)
    using value_type = modeValueType_t<Type, Mode>;

    // Composite structure for min/max entries
    struct Extrema
    {
        label cellID_;
        point position_;
        value_type value_;
    };

    // Limits are a min/max pair
    struct Limits
    {
        Extrema min_;
        Extrema max_;
    };

    // TBD:
    // static_assert(std::is_trivially_copyable_v<Limits>);
    // static_assert(std::is_standard_layout_v<Limits>);

    List<Limits> allLimits(numProc);

    // My proc-local entries
    auto& myData = allLimits[myProci];

    // Return the value or mag at specified index
    const auto getFieldValue = [](const UList<Type>& fld, label i)
    {
        if constexpr (Mode == modeType::mdMag)
        {
            return Foam::mag(fld[i]);
        }
        else
        {
            return fld[i];
        }
    };

    // Find min/max locations, possibly with on-the-fly mag() calculation
    const auto findMinMax_locations = [](const UList<Type>& fld)
    {
        if constexpr (Mode == modeType::mdMag)
        {
            return findMinMax_mag(fld);
        }
        else
        {
            return Foam::findMinMax(fld);
        }
    };


    // -----------------------------------------------------------------------

    // Find min/max info (internal field) - or use fallback values
    {
        const auto& centres = mesh_.C().primitiveField();
        const auto& fld = field.primitiveField();

        auto [minId, maxId] = findMinMax_locations(fld);

        // min
        if (auto& slot = myData.min_; minId >= 0)
        {
            slot.cellID_ = minId;
            slot.position_ = centres[minId];
            slot.value_ = getFieldValue(fld, minId);
        }
        else
        {
            slot.cellID_ = 0;
            slot.position_ = point::zero;
            slot.value_ = pTraits<value_type>::max;
        }

        // max
        if (auto& slot = myData.max_; maxId >= 0)
        {
            slot.cellID_ = maxId;
            slot.position_ = centres[maxId];
            slot.value_ = getFieldValue(fld, maxId);
        }
        else
        {
            slot.cellID_ = 0;
            slot.position_ = point::zero;
            slot.value_ = pTraits<value_type>::min;
        }
    }

    // Find min/max info (boundary field)
    if (!internal_)
    {
        const auto& fieldBoundary = field.boundaryField();
        const auto& CfBoundary = mesh_.C().boundaryField();

        forAll(fieldBoundary, patchi)
        {
            const Field<Type>& fld = fieldBoundary[patchi];

            if (fld.size())
            {
                const auto& centres = CfBoundary[patchi];
                const auto& faceCells
                    = fieldBoundary[patchi].patch().faceCells();

                auto [minId, maxId] = findMinMax_locations(fld);

                // min
                if (auto& slot = myData.min_; minId >= 0)
                {
                    if (auto val = getFieldValue(fld, minId); val < slot.value_)
                    {
                        slot.cellID_ = faceCells[minId];
                        slot.position_ = centres[minId];
                        slot.value_ = val;
                    }
                }

                // max
                if (auto& slot = myData.max_; maxId >= 0)
                {
                    if (auto val = getFieldValue(fld, maxId); slot.value_ < val)
                    {
                        slot.cellID_ = faceCells[maxId];
                        slot.position_ = centres[maxId];
                        slot.value_ = val;
                    }
                }
            }
        }
    }


    // Gather min/max data for all ranks.
    //
    // Handle as byte data directly. Although all elements of Extrema are
    // contiguous, and thus Limits is also contiguous, there is no
    // corresponding is_contiguous() trait for either of them.

    // Allgather with one Limits element per rank:
    UPstream::mpiAllGather(allLimits.data_bytes(), sizeof(Limits));


    // Find min/max across all processors
    label minProci = 0, maxProci = 0;

    for (label proci = 1; proci < numProc; ++proci)
    {
        const auto& [localMin, localMax] = allLimits[proci];

        // min
        if
        (
            const auto& best = allLimits[minProci].min_;
            localMin.value_ < best.value_
        )
        {
            minProci = proci;
        }

        // max
        if
        (
            const auto& best = allLimits[maxProci].max_;
            best.value_ < localMax.value_
        )
        {
            maxProci = proci;
        }
    }

    const auto& minData = allLimits[minProci].min_;
    const auto& maxData = allLimits[maxProci].max_;


    output
    (
        (outputFieldName.empty() ? field.name() : outputFieldName),
        minData.cellID_,
        maxData.cellID_,
        minData.position_,
        maxData.position_,
        minProci,
        maxProci,
        minData.value_,
        maxData.value_
    );
}


template<class Type>
bool Foam::functionObjects::fieldMinMax::calcMinMaxFields
(
    const word& fieldName,
    const modeType mode
)
{
    typedef GeometricField<Type, fvPatchField, volMesh> VolFieldType;

    const auto* fieldp = obr_.cfindObject<VolFieldType>(fieldName);

    if (fieldp)
    {
        const auto& field = *fieldp;

        switch (mode)
        {
            case modeType::mdMag :
            {
                calcMinMaxFieldType<Type, modeType::mdMag>
                (
                    field,
                    word("mag(" + fieldName + ")")
                );
                break;
            }
            case modeType::mdCmpt :
            {
                calcMinMaxFieldType(field, fieldName);
                break;
            }
            default:
            {
                FatalErrorInFunction
                    << "Unknown min/max mode: " << modeTypeNames_[mode_]
                    << exit(FatalError);
            }
        }
    }

    return bool(fieldp);
}


// ************************************************************************* //
