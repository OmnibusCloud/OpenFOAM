/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2019-2021,2026 OpenCFD Ltd.
    Copyright (C) YEAR AUTHOR, AFFILIATION
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

#include "fixedValuePointPatchFieldTemplate.H"
#include "addToRunTimeSelectionTable.H"
#include "pointPatchFieldMapper.H"
#include "pointFields.H"
#include "unitConversion.H"
#include "PatchFunction1.H"

//{{{ begin codeInclude
${codeInclude}
//}}} end codeInclude


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * Local Functions * * * * * * * * * * * * * * //

//{{{ begin localCode
${localCode}
//}}} end localCode


// * * * * * * * * * * * * * * * Global Functions  * * * * * * * * * * * * * //

// dynamicCode:
// SHA1 = ${SHA1sum}
//
// unique function name that can be checked if the correct library version
// has been loaded
extern "C" void ${typeName}_${SHA1sum}(bool load)
{
    if (load)
    {
        // Code that can be explicitly executed after loading
    }
    else
    {
        // Code that can be explicitly executed before unloading
    }
}

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

makePointPatchTypeField
(
    pointPatch${FieldType},
    ${typeName}FixedValuePointPatch${FieldType}
);

} // End namespace Foam


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::
${typeName}FixedValuePointPatch${FieldType}::
${typeName}FixedValuePointPatch${FieldType}
(
    const pointPatch& p,
    const DimensionedField<${TemplateType}, pointMesh>& iF
)
:
    parent_bctype(p, iF)
{
    if constexpr (${verbose:-false})
    {
        printMessage("Construct ${typeName} : patch/DimensionedField");
    }
}


Foam::
${typeName}FixedValuePointPatch${FieldType}::
${typeName}FixedValuePointPatch${FieldType}
(
    const this_bctype& pfld,
    const pointPatch& p,
    const DimensionedField<${TemplateType}, pointMesh>& iF,
    const pointPatchFieldMapper& mapper
)
:
    parent_bctype(pfld, p, iF, mapper)
{
    if constexpr (${verbose:-false})
    {
        printMessage("Construct ${typeName} : patch/DimensionedField/mapper");
    }
}


Foam::
${typeName}FixedValuePointPatch${FieldType}::
${typeName}FixedValuePointPatch${FieldType}
(
    const pointPatch& p,
    const DimensionedField<${TemplateType}, pointMesh>& iF,
    const dictionary& dict,
    IOobjectOption::readOption requireValue
)
:
    parent_bctype(p, iF, dict, requireValue)
{
    if constexpr (${verbose:-false})
    {
        printMessage("Construct ${typeName} : patch/dictionary");
    }
}


Foam::
${typeName}FixedValuePointPatch${FieldType}::
${typeName}FixedValuePointPatch${FieldType}
(
    const this_bctype& pfld,
    const DimensionedField<${TemplateType}, pointMesh>& iF
)
:
    parent_bctype(pfld, iF)
{
    if constexpr (${verbose:-false})
    {
        printMessage("Copy construct ${typeName} with internal field");
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void
Foam::
${typeName}FixedValuePointPatch${FieldType}::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }

    if constexpr (${verbose:-false})
    {
        printMessage("updateCoeffs ${typeName}");
    }

//{{{ begin code
    ${code}
//}}} end code

    this->parent_bctype::updateCoeffs();
}


// ************************************************************************* //
