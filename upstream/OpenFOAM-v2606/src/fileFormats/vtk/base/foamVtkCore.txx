/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2025 OpenCFD Ltd.
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

#include "foamVtkCore.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
Type Foam::vtk::Tools::copyTuple(const Type& value)
{
    if constexpr
    (
        std::is_same_v<SymmTensor<float>, Type>
     || std::is_same_v<SymmTensor<double>, Type>
    )
    {
        // VTK order is (XX, YY, ZZ, XY, YZ, XZ)
        return Type
        (
            value.xx(), value.yy(), value.zz(),
            value.xy(), value.yz(),
            value.xz()
        );
    }
    else
    {
        return value;
    }
}


template<class FloatType, class Type>
const FloatType* Foam::vtk::Tools::copyTuple_impl
(
    FloatType output[],
    const Type& value
)
{
    if constexpr
    (
        std::is_same_v<SymmTensor<float>, Type>
     || std::is_same_v<SymmTensor<double>, Type>
    )
    {
        // VTK order is (XX, YY, ZZ, XY, YZ, XZ)
        output[0] = value.xx();
        output[1] = value.yy();
        output[2] = value.zz();
        output[3] = value.xy();
        output[4] = value.yz();
        output[5] = value.xz();
    }
    else if constexpr (is_vectorspace_v<Type>)
    {
        const auto* data = value.cdata();

        std::copy(data, (data + pTraits<Type>::nComponents), output);
    }
    else
    {
        for (direction i = 0; i < pTraits<Type>::nComponents; ++i)
        {
            output[i] = component(value, i);
        }
    }

    return output;
}


template<class Type>
const float* Foam::vtk::Tools::copyTuple(float output[], const Type& value)
{
    return copyTuple_impl(output, value);
}


template<class Type>
const double* Foam::vtk::Tools::copyTuple(double output[], const Type& value)
{
    return copyTuple_impl(output, value);
}


template<class Type>
void Foam::vtk::Tools::reorderTuple(Type& value)
{
    if constexpr
    (
        std::is_same_v<SymmTensor<float>, Type>
     || std::is_same_v<SymmTensor<double>, Type>
    )
    {
        // OpenFOAM : (XX, XY, XZ, YY, YZ, ZZ)
        // VTK order: (XX, YY, ZZ, XY, YZ, XZ)

        auto* data = value.data();
        std::swap(data[1], data[3]);  // swap XY <-> YY
        std::swap(data[2], data[5]);  // swap XZ <-> ZZ
    }
}


// ************************************************************************* //
