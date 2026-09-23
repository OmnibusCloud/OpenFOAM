/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2013 OpenFOAM Foundation
    Copyright (C) 2018,2026 OpenCFD Ltd.
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

template<class T, class AccessOp>
Foam::labelList Foam::ListListOps::subSizes
(
    const UList<T>& lists,
    AccessOp aop
)
{
    labelList output(lists.size());
    auto out = output.begin();

    for (const auto& sub : lists)
    {
        *out = aop(sub).size();
        ++out;
    }

    return output;
}


template<class T, class AccessOp>
Foam::label Foam::ListListOps::sumSizes
(
    const UList<T>& lists,
    AccessOp aop
)
{
    label len = 0;

    for (const auto& sub : lists)
    {
        len += aop(sub).size();
    }

    return len;
}


template<class T>
Foam::label Foam::ListListOps::totalSize(const UList<T>& lists)
{
    label len = 0;

    for (const auto& sub : lists)
    {
        len += sub.size();
    }

    return len;
}


template<class ListType, class T>
ListType Foam::ListListOps::concat(List<T>& lists)
{
    label len = 0;

    for (const auto& sub : lists)
    {
        len += sub.size();
    }

    ListType output(len);
    output.resize(len);     // Consistent sizing (eg, DynamicList)

    auto out = output.begin();

    for (auto& sub : lists)
    {
        out = std::move(sub.begin(), sub.end(), out);
    }

    lists.clear();

    return output;
}


template<class AccessType, class T, class AccessOp>
AccessType Foam::ListListOps::combine
(
    const UList<T>& lists,
    AccessOp aop
)
{
    label len = 0;

    for (const auto& sub : lists)
    {
        len += aop(sub).size();
    }

    AccessType output(len);
    auto out = output.begin();

    for (const auto& sub : lists)
    {
        for (const auto& item : aop(sub))
        {
            *out = item;
            ++out;
        }
    }

    return output;
}


template<class AccessType, class T, class AccessOp, class OffsetOp>
AccessType Foam::ListListOps::combineOffset
(
    const UList<T>& lists,
    const labelUList& offsets,
    AccessOp aop,
    OffsetOp oop
)
{
    label len = 0;

    for (const auto& sub : lists)
    {
        len += aop(sub).size();
    }

    AccessType output(len);
    auto out = output.begin();
    auto off = offsets.begin();

    label offset = 0;
    for (const auto& sub : lists)
    {
        for (const auto& item : aop(sub))
        {
            *out = oop(item, offset);
            ++out;
        }

        offset += *off;
        ++off;
    }

    return output;
}


// ************************************************************************* //
