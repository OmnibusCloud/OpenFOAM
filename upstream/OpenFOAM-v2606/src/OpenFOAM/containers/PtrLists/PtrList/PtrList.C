/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2018-2025 OpenCFD Ltd.
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

#include "PtrList.H"
#include "SLPtrList.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

template<class T>
template<bool CheckSelf>
void Foam::PtrList<T>::copyPtrList(const UPtrList<T>& list)
{
    // Check for self-assignment here instead of caller
    if constexpr (CheckSelf)
    {
        if (FOAM_UNLIKELY(this == &list))
        {
            return;  // Self-assignment is a no-op
        }
    }

    const label len = list.size();

    // Truncate (frees old pointers) or extend the length
    PtrList<T>::resize(len);

    for (label i = 0; i < len; ++i)
    {
        if (const T* src = list.get(i); src)
        {
            if (this->ptrs_[i])
            {
                // Deep copy values into existing destination
                *(this->ptrs_[i]) = *src;
            }
            else
            {
                // Clone pointers for new entries
                this->ptrs_[i] = src->clone().ptr();
            }
        }
        else
        {
            // No source pointer, so remove destination (if any) too
            delete this->ptrs_[i];
            this->ptrs_[i] = nullptr;
        }
    }
}


// * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * * //

template<class T>
Foam::PtrList<T>::PtrList(const SLPtrList<T>& list)
:
    UPtrList<T>(list.size())
{
    if (list.size())
    {
        label i = 0;
        for (auto iter = list.cbegin(); iter != list.cend(); ++iter)
        {
            this->ptrs_[i++] = (*iter).clone().ptr();
        }
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

template<class T>
Foam::PtrList<T>::~PtrList()
{
    (this->ptrs_).free();  // Free old pointers
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class T>
template<class... Args>
Foam::PtrList<T> Foam::PtrList<T>::clone(Args&&... args) const
{
    const label len = this->size();

    PtrList<T> cloned(len);

    for (label i=0; i<len; ++i)
    {
        if (const T* ptr = this->ptrs_[i]; ptr)
        {
            cloned.ptrs_[i] = ptr->clone(std::forward<Args>(args)...).ptr();
        }
    }

    return cloned;
}


template<class T>
void Foam::PtrList<T>::resize(const label newLen)
{
    if (newLen <= 0)
    {
        clear();
    }
    else if (const label oldLen = this->size(); newLen != oldLen)
    {
        // Truncation frees old pointers
        for (label i = newLen; i < oldLen; ++i)
        {
            delete this->ptrs_[i];
            this->ptrs_[i] = nullptr;
        }

        // Any new elements are initialized to nullptr.
        (this->ptrs_).resize(newLen);
    }
}


// ************************************************************************* //
