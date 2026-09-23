/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2026 OpenCFD Ltd.
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

#include "globalIndex.H"
#include "ListOps.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
inline void Foam::vtk::write
(
    vtk::formatter& fmt,
    const Type& val,
    const label n
)
{
    if constexpr
    (
        std::is_same_v<Vector<float>, Type>
     || std::is_same_v<Vector<double>, Type>
    )
    {
        // Vector is frequently used
        for (label i = 0; i < n; ++i)
        {
            fmt.write(val.x());
            fmt.write(val.y());
            fmt.write(val.z());
        }
    }
    else if constexpr
    (
        std::is_same_v<SymmTensor<float>, Type>
     || std::is_same_v<SymmTensor<double>, Type>
    )
    {
        // VTK order is (XX, YY, ZZ, XY, YZ, XZ)
        for (label i = 0; i < n; ++i)
        {
            fmt.write(val.xx());
            fmt.write(val.yy());
            fmt.write(val.zz());
            fmt.write(val.xy());
            fmt.write(val.yz());
            fmt.write(val.xz());
        }
    }
    else if constexpr (is_vectorspace_v<Type>)
    {
        constexpr direction nCmpt = pTraits<Type>::nComponents;

        for (label i = 0; i < n; ++i)
        {
            for (direction cmpt = 0; cmpt < nCmpt; ++cmpt)
            {
                fmt.write(component(val, cmpt));
            }
        }
    }
    else
    {
        // label, scalar etc.

        for (label i = 0; i < n; ++i)
        {
            fmt.write(val);
        }
    }
}


template<class Type>
void Foam::vtk::writeValueParallel
(
    vtk::formatter& fmt,
    const Type& val,
    const label count
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }

    // Gather [value, count] tuples, including from master
    const auto values(UPstream::listGatherValues(val));
    const auto counts(UPstream::listGatherValues(count));

    if (UPstream::master())
    {
        forAll(counts, i)
        {
            // Write [value, count] tuple
            vtk::write(fmt, values[i], counts[i]);
        }
    }
}


template<class Type>
void Foam::vtk::writeList
(
    vtk::formatter& fmt,
    const UList<Type>& values
)
{
    for (const Type& val : values)
    {
        vtk::write(fmt, val);
    }
}


template<class Type, unsigned N>
void Foam::vtk::writeList
(
    vtk::formatter& fmt,
    const FixedList<Type, N>& values
)
{
    for (const Type& val : values)
    {
        vtk::write(fmt, val);
    }
}


template<class Type, class Addr>
void Foam::vtk::writeList
(
    vtk::formatter& fmt,
    const IndirectListBase<Type, Addr>& values
)
{
    const label len = values.size();

    for (label i = 0; i < len; ++i)
    {
        vtk::write(fmt, values[i]);
    }
}


template<class Type>
void Foam::vtk::writeList
(
    vtk::formatter& fmt,
    const UList<Type>& values,
    const labelUList& addressing
)
{
    for (auto idx : addressing)
    {
        vtk::write(fmt, values[idx]);
    }
}


template<class Type>
void Foam::vtk::writeList
(
    vtk::formatter& fmt,
    const UList<Type>& values,
    const bitSet& selected
)
{
    for (auto idx : selected)
    {
        vtk::write(fmt, values[idx]);
    }
}


template<class Type>
void Foam::vtk::writeLists
(
    vtk::formatter& fmt,
    const UList<Type>& values1,
    const UList<Type>& values2
)
{
    vtk::writeList(fmt, values1);
    vtk::writeList(fmt, values2);
}


template<class Type, class Addr>
void Foam::vtk::writeLists
(
    vtk::formatter& fmt,
    const UList<Type>& values1,
    const IndirectListBase<Type, Addr>& values2
)
{
    vtk::writeList(fmt, values1);
    vtk::writeList(fmt, values2);
}


template<class Type, class Addr>
void Foam::vtk::writeLists
(
    vtk::formatter& fmt,
    const IndirectListBase<Type, Addr>& values1,
    const UList<Type>& values2
)
{
    vtk::writeList(fmt, values1);
    vtk::writeList(fmt, values2);
}


template<class Type, class Addr>
void Foam::vtk::writeLists
(
    vtk::formatter& fmt,
    const IndirectListBase<Type, Addr>& values1,
    const IndirectListBase<Type, Addr>& values2
)
{
    vtk::writeList(fmt, values1);
    vtk::writeList(fmt, values2);
}


template<class Type>
void Foam::vtk::writeLists
(
    vtk::formatter& fmt,
    const UList<Type>& values1,
    const UList<Type>& values2,
    const labelUList& addressing2
)
{
    vtk::writeList(fmt, values1);
    vtk::writeList(fmt, values2, addressing2);
}


template<class Type>
void Foam::vtk::writeListParallel_subranks
(
    vtk::formatter& fmt,
    const UList<Type>& values
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }

    // The receive sizes
    const labelList recvSizes(globalIndex::calcRecvSizes(values.size()));

    if (UPstream::master())
    {
        // Receive and write
        const label maxRecvSize = recvSizes[0];
        if (!maxRecvSize)
        {
            return;  // Nothing to receive/write
        }
        DynamicList<Type> recvData(maxRecvSize);

        for (const int proci : UPstream::subProcs())
        {
            if (label procSize = recvSizes[proci]; procSize > 0)
            {
                recvData.resize_nocopy(procSize);
                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    proci,
                    recvData
                );
                vtk::writeList(fmt, recvData);
            }
        }
    }
    else
    {
        if (values.size())
        {
            UOPstream::write
            (
                UPstream::commsTypes::scheduled,
                UPstream::masterNo(),
                values
            );
        }
    }
}


template<class Type>
void Foam::vtk::writeListsParallel_subranks
(
    vtk::formatter& fmt,
    const UList<Type>& values1,
    const UList<Type>& values2
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }

    // The receive sizes
    const labelList recvSizes1(globalIndex::calcRecvSizes(values1.size()));
    const labelList recvSizes2(globalIndex::calcRecvSizes(values2.size()));

    if (UPstream::master())
    {
        // Receive and write
        const label maxRecvSize = std::max(recvSizes1[0], recvSizes2[0]);
        if (!maxRecvSize)
        {
            return;  // Nothing to receive/write
        }
        DynamicList<Type> recvData(maxRecvSize);

        for (const int proci : UPstream::subProcs())
        {
            // values1
            if (label procSize = recvSizes1[proci]; procSize > 0)
            {
                recvData.resize_nocopy(procSize);
                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    proci,
                    recvData
                );
                vtk::writeList(fmt, recvData);
            }

            // values2
            if (label procSize = recvSizes2[proci]; procSize > 0)
            {
                recvData.resize_nocopy(procSize);
                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    proci,
                    recvData
                );
                vtk::writeList(fmt, recvData);
            }
        }
    }
    else
    {
        if (values1.size())
        {
            UOPstream::write
            (
                UPstream::commsTypes::scheduled,
                UPstream::masterNo(),
                values1
            );
        }
        if (values2.size())
        {
            UOPstream::write
            (
                UPstream::commsTypes::scheduled,
                UPstream::masterNo(),
                values2
            );
        }
    }
}


template<class Type>
void Foam::vtk::writeListParallel
(
    vtk::formatter& fmt,
    const UList<Type>& values
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }

    if (UPstream::master())
    {
        // Write master data
        vtk::writeList(fmt, values);
    }

    vtk::writeListParallel_subranks(fmt, values);
}


template<class Type, class Addr>
void Foam::vtk::writeListParallel
(
    vtk::formatter& fmt,
    const IndirectListBase<Type, Addr>& values
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }

    List<Type> sendData;

    if (UPstream::master())
    {
        // Write master data
        vtk::writeList(fmt, values);
    }
    else
    {
        sendData = values.list();
    }

    vtk::writeListParallel_subranks(fmt, sendData);
}


template<class Type>
void Foam::vtk::writeListParallel
(
    vtk::formatter& fmt,
    const UList<Type>& values,
    const labelUList& addressing
)
{
    const UIndirectList<Type> list(values, addressing);
    vtk::writeListParallel(fmt, list);
}


template<class Type>
void Foam::vtk::writeListParallel
(
    vtk::formatter& fmt,
    const UList<Type>& values,
    const bitSet& selected
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }

    List<Type> sendData;

    if (UPstream::master())
    {
        // Write master data
        vtk::writeList(fmt, values, selected);
    }
    else
    {
        sendData = Foam::subset(selected, values);
    }

    vtk::writeListParallel_subranks(fmt, sendData);
}


template<class Type>
void Foam::vtk::writeListsParallel
(
    vtk::formatter& fmt,
    const UList<Type>& values1,
    const UList<Type>& values2
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }

    if (UPstream::master())
    {
        // Write master data
        vtk::writeList(fmt, values1);
        vtk::writeList(fmt, values2);
    }

    vtk::writeListsParallel_subranks(fmt, values1, values2);
}


template<class Type, class Addr>
void Foam::vtk::writeListsParallel
(
    vtk::formatter& fmt,
    const UList<Type>& values1,
    const IndirectListBase<Type, Addr>& values2
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }

    List<Type> sendData2;

    if (UPstream::master())
    {
        // Write master data
        vtk::writeList(fmt, values1);
        vtk::writeList(fmt, values2);
    }
    else
    {
        sendData2 = values2.list();
    }

    vtk::writeListsParallel_subranks(fmt, values1, sendData2);
}


template<class Type, class Addr>
void Foam::vtk::writeListsParallel
(
    vtk::formatter& fmt,
    const IndirectListBase<Type, Addr>& values1,
    const UList<Type>& values2
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }

    List<Type> sendData1;

    if (UPstream::master())
    {
        // Write master data
        vtk::writeList(fmt, values1);
        vtk::writeList(fmt, values2);
    }
    else
    {
        sendData1 = values1.list();
    }

    vtk::writeListsParallel_subranks(fmt, sendData1, values2);
}


template<class Type, class Addr>
void Foam::vtk::writeListsParallel
(
    vtk::formatter& fmt,
    const IndirectListBase<Type, Addr>& values1,
    const IndirectListBase<Type, Addr>& values2
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        // Non-contiguous data does not make sense
        FatalErrorInFunction
            << "Contiguous data only" << endl
            << Foam::exit(FatalError);
    }

    List<Type> sendData;

    if (UPstream::master())
    {
        // Write master data
        vtk::writeList(fmt, values1);
        vtk::writeList(fmt, values2);
    }
    else
    {
        // Flatten/concatenate both indirect lists
        const label len1 = values1.size();
        const label len2 = values2.size();

        sendData.resize(len1+len2);
        auto iter = sendData.begin();

        for (label i = 0; i < len1; ++i)
        {
            *iter++ = values1[i];
        }
        for (label i = 0; i < len2; ++i)
        {
            *iter++ = values2[i];
        }
    }

    vtk::writeListParallel_subranks(fmt, sendData);
}


template<class Type>
void Foam::vtk::writeListsParallel
(
    vtk::formatter& fmt,
    const UList<Type>& values1,
    const UList<Type>& values2,
    const labelUList& addressing2
)
{
    const UIndirectList<Type> list2(values2, addressing2);

    vtk::writeListsParallel(fmt, values1, list2);
}


// ************************************************************************* //
