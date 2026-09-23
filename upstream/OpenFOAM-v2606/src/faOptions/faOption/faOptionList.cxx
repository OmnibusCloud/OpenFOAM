/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2019-2025 OpenCFD Ltd.
------------------------------------------------------------------------------
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

#include "faOptionList.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace fa
{
    defineTypeNameAndDebug(optionList, 0);
}
}


// * * * * * * * * * * * * * Static Member Functions * * * * * * * * * * * * //

const Foam::dictionary& Foam::fa::optionList::optionsDict
(
    const dictionary& dict
)
{
    return dict.optionalSubDict("options", keyType::LITERAL);
}


// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

bool Foam::fa::optionList::readOptions(const dictionary& dict)
{
    checkTimeIndex_ = mesh_.time().timeIndex() + 2;

    bool allOk = true;
    for (fa::option& opt : *this)
    {
        bool ok = opt.read(dict.subDict(opt.name()));
        allOk = (allOk && ok);
    }
    return allOk;
}


void Foam::fa::optionList::checkApplied() const
{
    if (mesh_.time().timeIndex() == checkTimeIndex_)
    {
        for (const fa::option& opt : *this)
        {
            opt.checkApplied();
        }
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fa::optionList::optionList
(
    const fvMesh& mesh,
    const word& defaultAreaName
)
:
    PtrList<fa::option>(),
    mesh_(mesh),
    areaName_(defaultAreaName),
    checkTimeIndex_(mesh_.time().startTimeIndex() + 2)
{}


Foam::fa::optionList::optionList
(
    const fvMesh& mesh,
    const dictionary& dict,
    const word& defaultAreaName
)
:
    Foam::fa::optionList(mesh, defaultAreaName)
{
    reset(optionsDict(dict));
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::fa::optionList::reset(const dictionary& dict)
{
    // Count number of possible faOptions
    label count = 0;

    // Reject sub-dictionary entries that have an "area" entry that
    // conflicts with the expected name

    const word& expectedName = polyMesh::regionName(areaName_);

    const auto accept = [&](const dictionary& d)
    {
        if (expectedName.empty())
        {
            return true;
        }
        else if (auto* is = d.findStream("area", keyType::LITERAL))
        {
            const auto& tok = is->front();
            return
            (
                tok.isStringType()
             && fa::option::sameRegionNames(expectedName, tok.stringToken())
            );
        }
        else
        {
            return true;
        }
    };


    for (const entry& e : dict)
    {
        if (const auto* dictptr = e.dictPtr())
        {
            if (!accept(*dictptr))
            {
                // Produce a conspicuous warning message

                auto& err = IOWarningInFunction(*dictptr) << nl
                    << incrIndent
                    << indent
                    << "Ignoring faOption entry: " << e.keyword() << nl
                    << indent << "which has an inconsistent 'area' entry." << nl
                    << indent << nl
                    << incrIndent
                    << indent << "expected : " << expectedName << nl
                    << indent << "found    : ";

                if (auto* is = dictptr->findStream("area", keyType::LITERAL))
                {
                    err << is->front();
                }

                err << decrIndent << nl << nl
                    << indent
                    << "It is either located in the wrong faOptions" << nl
                    << indent
                    << "or the 'area' entry should be removed." << nl
                    << decrIndent << nl << endl;
            }
            else
            {
                ++count;
            }
        }
    }

    this->resize_null(count);

    count = 0;

    for (const entry& e : dict)
    {
        if (const auto* dictptr = e.dictPtr())
        {
            if (accept(*dictptr))
            {
                const word& name = e.keyword();
                const auto& coeffs = *dictptr;

                this->set
                (
                    count++,
                    fa::option::New(name, coeffs, mesh_, areaName_)
                );
            }
        }
    }
}


bool Foam::fa::optionList::appliesToField(const word& fieldName) const
{
    for (const fa::option& source : *this)
    {
        const label fieldi = source.applyToField(fieldName);

        if (fieldi != -1)
        {
            return true;
        }
    }

    return false;
}


bool Foam::fa::optionList::read(const dictionary& dict)
{
    return readOptions(optionsDict(dict));
}


bool Foam::fa::optionList::writeData(Ostream& os) const
{
    // Write list contents
    for (const fa::option& opt : *this)
    {
        os  << nl;
        opt.writeHeader(os);
        opt.writeData(os);
        opt.writeFooter(os);
    }

    // Check state of IOstream
    return os.good();
}


// * * * * * * * * * * * * * * * IOstream Operators  * * * * * * * * * * * * //

Foam::Ostream& Foam::operator<<(Ostream& os, const fa::optionList& options)
{
    options.writeData(os);
    return os;
}


// ************************************************************************* //
