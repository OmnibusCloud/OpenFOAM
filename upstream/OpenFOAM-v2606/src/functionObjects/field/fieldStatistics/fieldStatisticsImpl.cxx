/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2025 OpenCFD Ltd.
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

#include "fieldStatistics.H"
#include "volFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace
{

// Return min/max indices based on the magnitude of the input.
// This function should be migrated elsewhere.
template<class T>
Foam::labelPair findMinMax_mag
(
    const Foam::UList<T>& input,
    Foam::label start=0
)
{
    using namespace Foam;

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

} // End anonymous namespace


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class GeoField>
Foam::tmp<Foam::Field<typename GeoField::value_type>>
Foam::functionObjects::fieldStatistics::flatten(const GeoField& fld) const
{
    typedef typename GeoField::value_type value_type;
    typedef Field<value_type> FieldType;

    label n(0);

    if (!internal_)
    {
        // Count boundary values
        for (const auto& pfld : fld.boundaryField())
        {
            if (!pfld.coupled())
            {
                n += pfld.size();
            }
        }
    }

    if (!n)
    {
        // No boundary values - quick return
        return tmp<FieldType>(fld.primitiveField());
    }


    // Combined internal + flattened boundary fields
    // - this adds extra storage, but necessary since the visitor pattern
    //   requires a single input

    auto tflatFld = tmp<FieldType>::New(fld.size() + n);
    auto& flatFld = tflatFld.ref();

    // Copy internal values
    flatFld.slice(0, fld.size()) = fld.primitiveField();

    // Copy boundary values
    n = fld.size();
    for (const auto& pfld : fld.boundaryField())
    {
        if (!pfld.coupled())
        {
            flatFld.slice(n, pfld.size()) = pfld;
            n += pfld.size();
        }
    }

    return tflatFld;
}


template<class Type>
bool Foam::functionObjects::fieldStatistics::calcStat(const word& fieldName)
{
    typedef GeometricField<Type, fvPatchField, volMesh> VolFieldType;

    const auto* fieldp = obr_.cfindObject<VolFieldType>(fieldName);
    if (!fieldp)
    {
        return false;
    }
    const auto& field = *fieldp;

    tmp<Field<Type>> tfullfield = flatten(field);
    const auto& fullfield = tfullfield.cref();

    HashTable<variantOutput> result;
    for (const auto& iter : statistics_.csorted())
    {
        const statistic& stat = iter.val();

        // Assign a new entry, overwriting existing entries
        result.set(stat.name_, stat.calc(fullfield));
    }

    results_.set(fieldName, result);

    if (extrema_)
    {
        if (mode_ == modeType::MAG)
        {
            extremaResults_.set
            (
                fieldName,
                calcExtremaData<VolFieldType, modeType::MAG>(field)
            );
        }
        else
        {
            extremaResults_.set
            (
                fieldName,
                calcExtremaData(field)
            );
        }
    }

    return true;
}


template<class T>
T Foam::functionObjects::fieldStatistics::calcMean(const Field<T>& field) const
{
    if (internal_ && (mean_ == meanType::VOLUMETRIC))
    {
        return gWeightedAverage(mesh_.V(), field);
    }

    return gAverage(field);
}


template<class T, Foam::functionObjects::fieldStatistics::modeType Mode>
Foam::functionObjects::fieldStatistics::modeValueType_t<T, Mode>
Foam::functionObjects::fieldStatistics::calcMin(const Field<T>& field) const
{
    if constexpr (Mode == modeType::MAG)
    {
        scalar limit = pTraits<scalar>::max;
        for (const auto& elem : field)
        {
            if (const scalar val = Foam::mag(elem); val < limit)
            {
                limit = val;
            }
        }
        Foam::reduce(limit, minOp<scalar>());
        return limit;
    }
    else
    {
        return gMin(field);
    }
}


template<class T, Foam::functionObjects::fieldStatistics::modeType Mode>
Foam::functionObjects::fieldStatistics::modeValueType_t<T, Mode>
Foam::functionObjects::fieldStatistics::calcMax(const Field<T>& field) const
{
    if constexpr (Mode == modeType::MAG)
    {
        scalar limit = pTraits<scalar>::min;
        for (const auto& elem : field)
        {
            if (const scalar val = Foam::mag(elem); limit < val)
            {
                limit = val;
            }
        }
        Foam::reduce(limit, maxOp<scalar>());
        return limit;
    }
    else
    {
        return gMax(field);
    }
}


template<class T>
T Foam::functionObjects::fieldStatistics::calcVariance
(
    const Field<T>& field
) const
{
    const T avg(calcMean(field));

    T var = Zero;
    for (const auto& elem : field)
    {
        var += (elem - avg);
    }

    label count = field.size();
    Foam::sumReduce(var, count);

    if (count <= 1)
    {
        return Zero;
    }

    return 1.0/(count - 1.0)*var;
}


template<class GeoField, Foam::functionObjects::fieldStatistics::modeType Mode>
Foam::Pair<Foam::functionObjects::fieldStatistics::extremaData>
Foam::functionObjects::fieldStatistics::calcExtremaData
(
    const GeoField& field
) const
{
    // The data type of the geometric field
    typedef typename GeoField::value_type Type;

    const auto myProci = UPstream::myProcNo();
    const auto numProc = UPstream::nProcs();

    // NOTE: the code here is largely identical to fieldMinMax
    // For modeType::MAG, we compute on magnitudes (scalar values)

    // Magnitude mode: scalar; Component mode: Field value_type
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
        if constexpr (Mode == modeType::MAG)
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
        if constexpr (Mode == modeType::MAG)
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

        auto [minId, maxId] = findMinMax_locations(field);

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
        // Find extrema within the boundary fields
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

    // Results
    Pair<extremaData> results;

    // min
    {
        const auto& limit = allLimits[minProci].min_;
        auto& slot = results.first();

        slot.value_ = limit.value_;
        slot.procID_ = minProci;
        slot.cellID_ = limit.cellID_;
        slot.position_ = limit.position_;
    }

    // max
    {
        const auto& limit = allLimits[maxProci].max_;
        auto& slot = results.second();

        slot.value_ = limit.value_;
        slot.procID_ = maxProci;
        slot.cellID_ = limit.cellID_;
        slot.position_ = limit.position_;
    }

    return results;
}


// ************************************************************************* //
