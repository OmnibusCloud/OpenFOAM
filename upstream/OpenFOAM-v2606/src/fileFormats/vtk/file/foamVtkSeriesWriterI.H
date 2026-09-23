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

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

inline Foam::vtk::seriesWriter::seriesWriter() noexcept
:
    extType_(endingType::SERIES)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

inline bool Foam::vtk::seriesWriter::empty() const noexcept
{
    return entries_.empty();
}


inline Foam::label Foam::vtk::seriesWriter::size() const noexcept
{
    return entries_.size();
}


inline bool Foam::vtk::seriesWriter::json() const noexcept
{
    return (extType_ == endingType::JSON);
}


inline bool Foam::vtk::seriesWriter::json(bool on) noexcept
{
    bool old(extType_ == endingType::JSON);
    extType_ = (on ? endingType::JSON : endingType::SERIES);
    return old;
}


inline void Foam::vtk::seriesWriter::clear()
{
    entries_.clear();
    existing_.clear();
}


inline bool Foam::vtk::seriesWriter::append(const fileNameInstant& inst)
{
    // Strip out path before saving
    return appendCheck(fileNameInstant(inst.value(), inst.name().name()));
}


inline bool Foam::vtk::seriesWriter::append(fileNameInstant&& inst)
{
    // Strip out path before saving
    inst.name().remove_path();

    return appendCheck(inst);
}


inline bool Foam::vtk::seriesWriter::append
(
    scalar timeValue,
    const fileName& file
)
{
    // Strip out path before saving
    return appendCheck(fileNameInstant(timeValue, file.name()));
}


inline bool Foam::vtk::seriesWriter::append
(
    scalar timeValue,
    fileName&& file
)
{
    // Strip out path before saving
    file.remove_path();

    return appendCheck(fileNameInstant(timeValue, std::move(file)));
}


inline void Foam::vtk::seriesWriter::print(Ostream& os) const
{
    seriesWriter::print(os, entries_);
}


// ************************************************************************* //
