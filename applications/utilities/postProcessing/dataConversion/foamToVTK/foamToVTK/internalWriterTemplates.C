/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2024 OpenFOAM Foundation
     \\/     M anipulation  |
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

#include "internalWriter.H"
#include "vtkWriteFieldOps.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type, class GeoMesh>
void Foam::internalWriter::write
(
    const UPtrList<const DimensionedField<Type, GeoMesh>>& flds
)
{
    forAll(flds, i)
    {
        vtkWriteOps::write(os_, binary_, flds[i], vMesh_);
    }
}


template<class Type, class GeoMesh>
void Foam::internalWriter::write
(
    const UPtrList<const GeometricField<Type, GeoMesh>>& flds
)
{
    forAll(flds, i)
    {
        vtkWriteOps::write(os_, binary_, flds[i], vMesh_);
    }
}


template<class Type>
void Foam::internalWriter::write
(
    const volPointInterpolation& pInterp,
    const UPtrList<const VolField<Type>>& flds
)
{
    forAll(flds, i)
    {
        vtkWriteOps::write
        (
            os_,
            binary_,
            flds[i],
            pInterp.interpolate(flds[i])(),
            vMesh_
        );
    }
}


// ************************************************************************* //
