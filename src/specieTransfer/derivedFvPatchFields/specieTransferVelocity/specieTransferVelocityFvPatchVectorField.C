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

#include "specieTransferVelocityFvPatchVectorField.H"
#include "specieTransferMassFractionFvPatchScalarField.H"
#include "addToRunTimeSelectionTable.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "fluidMulticomponentThermo.H"
#include "basicThermo.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::specieTransferVelocityFvPatchVectorField::
specieTransferVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    fixedValueInletOutletFvPatchVectorField(p, iF, dict),
    rhoName_(dict.lookupOrDefault<word>("rho", "rho"))
{}


Foam::specieTransferVelocityFvPatchVectorField::
specieTransferVelocityFvPatchVectorField
(
    const specieTransferVelocityFvPatchVectorField& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fieldMapper& mapper
)
:
    fixedValueInletOutletFvPatchVectorField(ptf, p, iF, mapper),
    rhoName_(ptf.rhoName_)
{}


Foam::specieTransferVelocityFvPatchVectorField::
specieTransferVelocityFvPatchVectorField
(
    const specieTransferVelocityFvPatchVectorField& ptf,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueInletOutletFvPatchVectorField(ptf, iF),
    rhoName_(ptf.rhoName_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

const Foam::tmp<Foam::scalarField>
Foam::specieTransferVelocityFvPatchVectorField::phip() const
{
    typedef specieTransferMassFractionFvPatchScalarField YBCType;
    const PtrList<volScalarField>& Y = YBCType::thermo(db()).Y();

    // Sum up the phiYp-s from all the species
    tmp<scalarField> tPhip(new scalarField(this->size(), 0));
    scalarField& phip = tPhip.ref();
    forAll(Y, i)
    {
        const fvPatchScalarField& Yp = Y[i].boundaryField()[patch().index()];

        if (!isA<YBCType>(Yp))
        {
            FatalErrorInFunction
                << "The mass-fraction condition on patch " << patch().name()
                << " is not of type " << YBCType::typeName << "."
                << exit(FatalError);
        }

        phip += refCast<const YBCType>(Yp).phiYp();
    }

    return tPhip;
}


void Foam::specieTransferVelocityFvPatchVectorField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    // Get the density
    const scalarField& rhop =
        patch().lookupPatchField<volScalarField, scalar>(rhoName_);

    // Set the normal component of the velocity to match the computed flux
    const vectorField nf(patch().nf());
    const tensorField Tau(tensor::I - sqr(nf));
    this->operator==((Tau & *this) + nf*phip()/(rhop*patch().magSf()));

    fixedValueInletOutletFvPatchVectorField::updateCoeffs();
}


void Foam::specieTransferVelocityFvPatchVectorField::write
(
    Ostream& os
) const
{
    fvPatchVectorField::write(os);
    writeEntryIfDifferent<word>(os, "rho", "rho", rhoName_);
    writeEntry(os, "value", *this);
}


// * * * * * * * * * * * * * * Build Macro Function  * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchVectorField,
        specieTransferVelocityFvPatchVectorField
    );
}

// ************************************************************************* //
