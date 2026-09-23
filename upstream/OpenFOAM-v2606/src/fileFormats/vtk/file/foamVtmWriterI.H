/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

template<Foam::vtk::fileTag FileType, Foam::vtk::fileTag DataType>
inline bool Foam::vtk::vtmWriter::appendTyped
(
    const fileName& file
)
{
    bool ok = append(file, FileType);
    if (ok) entries_.back().setDataType(DataType);
    return ok;
}


template<Foam::vtk::fileTag FileType, Foam::vtk::fileTag DataType>
inline bool Foam::vtk::vtmWriter::appendTyped
(
    const word& name,
    const fileName& file
)
{
    bool ok = append(name, file, FileType);
    if (ok) entries_.back().setDataType(DataType);
    return ok;
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

inline Foam::word Foam::vtk::vtmWriter::ext()
{
    return vtk::fileExtension[vtk::fileTag::MULTI_BLOCK];
}


inline bool Foam::vtk::vtmWriter::append_poly(const fileName& file)
{
    return appendTyped<vtk::fileTag::POLY_DATA>(file);
}


inline bool Foam::vtk::vtmWriter::append_poly
(
    const word& name,
    const fileName& file
)
{
    return appendTyped<vtk::fileTag::POLY_DATA>(name, file);
}


inline bool Foam::vtk::vtmWriter::append_ugrid(const fileName& file)
{
    return appendTyped<vtk::fileTag::UNSTRUCTURED_GRID>(file);
}


inline bool Foam::vtk::vtmWriter::append_ugrid
(
    const word& name,
    const fileName& file
)
{
    return appendTyped<vtk::fileTag::UNSTRUCTURED_GRID>(name, file);
}


template<Foam::vtk::fileTag DataType>
inline bool Foam::vtk::vtmWriter::append_hdf(const fileName& file)
{
    return appendTyped<vtk::fileTag::VTK_HDF, DataType>(file);
}


template<Foam::vtk::fileTag DataType>
inline bool Foam::vtk::vtmWriter::append_hdf
(
    const word& name,
    const fileName& file
)
{
    return appendTyped<vtk::fileTag::VTK_HDF, DataType>(name, file);
}


// ************************************************************************* //
