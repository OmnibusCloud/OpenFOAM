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

#include "fileName.H"
#include "UPstreamFile.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                    Class UPstream::File::Impl Declaration
\*---------------------------------------------------------------------------*/

class UPstream::File::Impl {};

} // End namespace Foam


// * * * * * * * * * * * * * Static Member Functions * * * * * * * * * * * * //

bool Foam::UPstream::File::supported()
{
    return false;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::UPstream::File::File() {}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::UPstream::File::~File()
{}  // Non-default in header (incomplete types)


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

const Foam::fileName& Foam::UPstream::File::name() const
{
    return fileName::null;
}


bool Foam::UPstream::File::is_open() const
{
    return false;
}


bool Foam::UPstream::File::close()
{
    return false;
}


// * * * * * * * * * * * * Member Functions (Reading)  * * * * * * * * * * * //

#if 0
bool Foam::UPstream::File::open_read
(
    const int communicator,
    const fileName& pathname
)
{
    NotImplemented;
    return false;
}
#endif


// * * * * * * * * * * * * Member Functions (Writing)  * * * * * * * * * * * //

bool Foam::UPstream::File::open_write
(
    const int communicator,
    const fileName& pathname,
    IOstreamOption::atomicType
)
{
    NotImplemented;
    return false;
}


bool Foam::UPstream::File::write_data
(
    const void* data,
    std::streamsize count,
    const UPstream::dataTypes dataTypeId
)
{
    NotImplemented;
    return false;
}


bool Foam::UPstream::File::write_data_at
(
    std::streamsize offset,
    const void* data,
    std::streamsize count,
    const UPstream::dataTypes dataTypeId
)
{
    NotImplemented;
    return false;
}


bool Foam::UPstream::File::write_data_all
(
    const void* data,
    std::streamsize count,
    const UPstream::dataTypes dataTypeId
)
{
    NotImplemented;
    return false;
}


bool Foam::UPstream::File::write_data_at_all
(
    std::streamsize offset,
    const void* data,
    std::streamsize count,
    const UPstream::dataTypes dataTypeId
)
{
    NotImplemented;
    return false;
}


bool Foam::UPstream::File::set_size(std::streamsize num_bytes)
{
    NotImplemented;
    return false;
}


// ************************************************************************* //
