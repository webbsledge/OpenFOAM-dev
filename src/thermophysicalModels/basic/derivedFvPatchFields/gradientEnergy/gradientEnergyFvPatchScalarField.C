/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2025 OpenFOAM Foundation
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

#include "zeroGradientFvPatchFields.H"
#include "gradientEnergyFvPatchScalarField.H"
#include "gradientEnergyCalculatedTemperatureFvPatchScalarField.H"
#include "addToRunTimeSelectionTable.H"
#include "fieldMapper.H"
#include "volFields.H"
#include "basicThermo.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::gradientEnergyFvPatchScalarField::gradientEnergyFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedGradientFvPatchScalarField(p, iF)
{
    gradient() = scalar(0);
}


Foam::gradientEnergyFvPatchScalarField::gradientEnergyFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    fixedGradientFvPatchScalarField(p, iF, dict)
{}


Foam::gradientEnergyFvPatchScalarField::gradientEnergyFvPatchScalarField
(
    const gradientEnergyFvPatchScalarField& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fieldMapper& mapper
)
:
    fixedGradientFvPatchScalarField(ptf, p, iF, mapper, false)
{
    map(ptf, mapper);
}


Foam::gradientEnergyFvPatchScalarField::gradientEnergyFvPatchScalarField
(
    const gradientEnergyFvPatchScalarField& tppsf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedGradientFvPatchScalarField(tppsf, iF)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::gradientEnergyFvPatchScalarField::map
(
    const gradientEnergyFvPatchScalarField& ptf,
    const fieldMapper& mapper
)
{
    // Unmapped faces are considered zero-gradient/adiabatic
    // until they are corrected later
    mapper(*this, ptf, [&](){ return this->patchInternalField(); });
    mapper(gradient(), ptf.gradient(), scalar(0));
}


void Foam::gradientEnergyFvPatchScalarField::map
(
    const fvPatchScalarField& ptf,
    const fieldMapper& mapper
)
{
    map(refCast<const gradientEnergyFvPatchScalarField>(ptf), mapper);
}


void Foam::gradientEnergyFvPatchScalarField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    const basicThermo& thermo = basicThermo::lookupThermo(*this);
    const label patchi = patch().index();

    fvPatchScalarField& Tp =
        const_cast<fvPatchScalarField&>(thermo.T().boundaryField()[patchi]);

    Tp.evaluate();

    if
    (
        isA<zeroGradientFvPatchScalarField>(Tp)
     || isA<fixedGradientFvPatchScalarField>(Tp)
    )
    {
        gradient() =
            thermo.Cpv(Tp, patchi)*Tp.snGrad()
          + patch().deltaCoeffs()*
            (
                thermo.he(Tp, patchi)
              - thermo.he(Tp, patch().faceCells())
            );
    }
    else if (isA<gradientEnergyCalculatedTemperatureFvPatchScalarField>(Tp))
    {
        gradientEnergyCalculatedTemperatureFvPatchScalarField& Tg =
            refCast<gradientEnergyCalculatedTemperatureFvPatchScalarField>(Tp);

        gradient() = Tg.heGradient();
    }
    else
    {
        FatalErrorInFunction
            << "Temperature boundary condition not recognised."
            << "A " << typeName << " condition for energy must be used with a "
            << zeroGradientFvPatchScalarField::typeName << ", "
            << fixedGradientFvPatchScalarField::typeName << " or "
            << gradientEnergyCalculatedTemperatureFvPatchScalarField::typeName
            << " condition for temperature."
            << exit(FatalError);
    }

    fixedGradientFvPatchScalarField::updateCoeffs();
}


void Foam::gradientEnergyFvPatchScalarField::write(Ostream& os) const
{
    fixedGradientFvPatchScalarField::write(os);
    writeEntry(os, "value", *this);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makeNullConstructablePatchTypeField
    (
        fvPatchScalarField,
        gradientEnergyFvPatchScalarField
    );
}

// ************************************************************************* //
