/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2025 OpenFOAM Foundation
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

#include "surfaceAreaLagrangianScalarFieldSource.H"
#include "shaped.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::surfaceAreaLagrangianScalarFieldSource::
surfaceAreaLagrangianScalarFieldSource
(
    const regIOobject& iIo
)
:
    LagrangianScalarFieldSource(iIo),
    CloudLagrangianFieldSource<scalar>(*this)
{}


Foam::surfaceAreaLagrangianScalarFieldSource::
surfaceAreaLagrangianScalarFieldSource
(
    const regIOobject& iIo,
    const dictionary& dict
)
:
    LagrangianScalarFieldSource(iIo, dict),
    CloudLagrangianFieldSource<scalar>(*this)
{}


Foam::surfaceAreaLagrangianScalarFieldSource::
surfaceAreaLagrangianScalarFieldSource
(
    const surfaceAreaLagrangianScalarFieldSource& field,
    const regIOobject& iIo
)
:
    LagrangianScalarFieldSource(field, iIo),
    CloudLagrangianFieldSource<scalar>(*this)
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::surfaceAreaLagrangianScalarFieldSource::
~surfaceAreaLagrangianScalarFieldSource()
{}


// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

Foam::tmp<Foam::LagrangianSubScalarField>
Foam::surfaceAreaLagrangianScalarFieldSource::value
(
    const LagrangianInjection& injection,
    const LagrangianSubMesh& subMesh
) const
{
    return cloud<clouds::shaped>(injection, subMesh).a(injection, subMesh);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makeNullConstructableLagrangianTypeFieldSource
    (
        LagrangianScalarFieldSource,
        surfaceAreaLagrangianScalarFieldSource
    );
}

// ************************************************************************* //
