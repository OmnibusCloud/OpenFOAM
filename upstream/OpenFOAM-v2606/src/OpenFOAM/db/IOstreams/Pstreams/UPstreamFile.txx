/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2025 Mark Olesen
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

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

inline bool Foam::UPstream::File::write
(
    std::string_view sv
)
{
    if (sv.empty())
    {
        // no-op for no content
        return true;
    }

    return this->write_data
    (
        sv.data(), sv.size(),
        UPstream::dataTypes::type_byte
    );
}


template<class Type>
bool Foam::UPstream::File::write
(
    const Type* buffer,
    std::streamsize count
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        FatalErrorInFunction
            << "Only contiguous data can be supported!"
            << Foam::abort(FatalError);
        return false;
    }
    else if (buffer && count > 1)
    {
        // Use element or component type (or byte-wise) for data type
        return this->write_data
        (
            buffer,  // The data or cmpt pointer
            UPstream_dataType<Type>::size(count),
            UPstream_dataType<Type>::datatype_id
        );
    }
    else
    {
        // no-op for no content
        return true;
    }
}


inline bool Foam::UPstream::File::write_at
(
    std::streamsize offset,
    std::string_view sv
)
{
    if (sv.empty())
    {
        // no-op for no content
        return true;
    }

    return this->write_data_at
    (
        offset,
        sv.data(), sv.size(),
        UPstream::dataTypes::type_byte
    );
}


template<class Type>
bool Foam::UPstream::File::write_at
(
    std::streamsize offset,
    const Type* buffer,
    std::streamsize count
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        FatalErrorInFunction
            << "Only contiguous data can be supported!"
            << Foam::abort(FatalError);
        return false;
    }
    else if (buffer && count > 1)
    {
        // Use element or component type (or byte-wise) for data type
        return this->write_data_at
        (
            offset,
            buffer,  // The data or cmpt pointer
            UPstream_dataType<Type>::size(count),
            UPstream_dataType<Type>::datatype_id
        );
    }
    else
    {
        // no-op for no content
        return true;
    }
}


inline bool Foam::UPstream::File::write_all
(
    std::string_view sv
)
{
    return this->write_data_all
    (
        sv.data(), sv.size(),
        UPstream::dataTypes::type_byte
    );
}


template<class Type>
bool Foam::UPstream::File::write_all
(
    const Type* buffer,
    std::streamsize count
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        FatalErrorInFunction
            << "Only contiguous data can be supported!"
            << Foam::abort(FatalError);
        return false;
    }
    else
    {
        // Use element or component type (or byte-wise) for data type
        return this->write_data_all
        (
            buffer,  // The data or cmpt pointer
            UPstream_dataType<Type>::size(count),
            UPstream_dataType<Type>::datatype_id
        );
    }
}


inline bool Foam::UPstream::File::write_at_all
(
    std::streamsize offset,
    std::string_view sv
)
{
    return this->write_data_at_all
    (
        offset,
        sv.data(), sv.size(),
        UPstream::dataTypes::type_byte
    );
}


template<class Type>
bool Foam::UPstream::File::write_at_all
(
    std::streamsize offset,
    const Type* buffer,
    std::streamsize count
)
{
    if constexpr (!is_contiguous_v<Type>)
    {
        FatalErrorInFunction
            << "Only contiguous data can be supported!"
            << Foam::abort(FatalError);
        return false;
    }
    else
    {
        // Use element or component type (or byte-wise) for data type
        return this->write_data_at_all
        (
            offset,
            buffer,  // The data or cmpt pointer
            UPstream_dataType<Type>::size(count),
            UPstream_dataType<Type>::datatype_id
        );
    }
}


// ************************************************************************* //
