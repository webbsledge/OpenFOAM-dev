/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2019-2025 OpenFOAM Foundation
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

#include "spherical.H"
#include "populationBalanceModel.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace populationBalance
{
namespace shapeModels
{
    defineTypeNameAndDebug(spherical, 0);
    addToRunTimeSelectionTable
    (
        shapeModel,
        spherical,
        dictionary
    );
}
}
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::populationBalance::shapeModels::spherical::spherical
(
    const dictionary& dict,
    const populationBalanceModel& popBal
)
:
    shapeModel(popBal)
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::populationBalance::shapeModels::spherical::~spherical()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::tmp<Foam::volScalarField>
Foam::populationBalance::shapeModels::spherical::a(const label i) const
{
    return
        volScalarField::New
        (
            "a" + Foam::name(i),
            popBal().mesh(),
            6/popBal().dSphs()[i]*popBal().vs()[i]
        );
}


Foam::tmp<Foam::volScalarField>
Foam::populationBalance::shapeModels::spherical::d(const label i) const
{
    return
        volScalarField::New
        (
            "d" + Foam::name(i),
            popBal().mesh(),
            popBal().dSphs()[i]
        );
}


// ************************************************************************* //
