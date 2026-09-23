/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2019-2022 OpenCFD Ltd.
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

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class T>
inline Foam::BiIndirectList<T>::BiIndirectList
(
    const UList<T>& posList,
    const UList<T>& negList,
    const labelUList& addr
)
:
    posList_(const_cast<UList<T>&>(posList)),
    negList_(const_cast<UList<T>&>(negList)),
    addr_(addr)
{}


template<class T>
inline Foam::BiIndirectList<T>::BiIndirectList
(
    const UList<T>& posList,
    const UList<T>& negList,
    labelList&& addr
)
:
    posList_(const_cast<UList<T>&>(posList)),
    negList_(const_cast<UList<T>&>(negList)),
    addr_(std::move(addr))
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class T>
inline void Foam::BiIndirectList<T>::resetAddressing(const labelUList& addr)
{
    addr_ = addr;
}


template<class T>
inline void Foam::BiIndirectList<T>::resetAddressing(labelList&& addr)
{
    addr_ = std::move(addr);
}


template<class T>
inline void Foam::BiIndirectList<T>::extract(UList<T>& result) const
{
    #ifdef FULLDEBUG
    if (FOAM_UNLIKELY(addr_.size() < result.size()))
    {
        FatalErrorInFunction
            << "result size = " << result.size()
            << " but addressing size = " << addr_.size() << nl
            << abort(FatalError);
    }
    #endif

    const label len = std::min(addr_.size(), result.size());

    for (label i = 0; i < len; ++i)
    {
        result[i] = (*this)[i];
    }
}


template<class T>
inline Foam::List<T> Foam::BiIndirectList<T>::list() const
{
    List<T> result(addr_.size());
    this->extract(result);
    return result;
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

template<class T>
inline Foam::List<T> Foam::BiIndirectList<T>::operator()() const
{
    List<T> result(addr_.size());
    this->extract(result);
    return result;
}


template<class T>
inline T& Foam::BiIndirectList<T>::operator[](const label i)
{
    const label index = addr_[i];

    return (index >= 0 ? posList_[index] : negList_[-index-1]);
}


template<class T>
inline const T& Foam::BiIndirectList<T>::operator[](const label i) const
{
    const label index = addr_[i];

    return (index >= 0 ? posList_[index] : negList_[-index-1]);
}



template<class T>
inline void Foam::BiIndirectList<T>::operator=(const T& val)
{
    const label len = size();

    for (label i = 0; i < len; ++i)
    {
        (*this)[i] = val;
    }
}


template<class T>
inline void Foam::BiIndirectList<T>::operator=(Foam::zero)
{
    const label len = size();

    for (label i = 0; i < len; ++i)
    {
        (*this)[i] = Foam::zero{};
    }
}


template<class T>
inline void Foam::BiIndirectList<T>::operator=(const UList<T>& vals)
{
    if (FOAM_UNLIKELY(addr_.size() != vals.size()))
    {
        FatalErrorInFunction
            << "Addressing and list of addressed elements "
               "have different sizes: "
            << addr_.size() << " " << vals.size()
            << abort(FatalError);
    }

    const label len = size();

    for (label i = 0; i < len; ++i)
    {
        (*this)[i] = vals[i];
    }
}


// ************************************************************************* //
