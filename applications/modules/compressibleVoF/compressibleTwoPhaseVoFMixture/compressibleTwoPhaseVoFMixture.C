/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2013-2025 OpenFOAM Foundation
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

#include "compressibleTwoPhaseVoFMixture.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(compressibleTwoPhaseVoFMixture, 0);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::compressibleTwoPhaseVoFMixture::compressibleTwoPhaseVoFMixture
(
    const fvMesh& mesh
)
:
    twoPhaseVoFMixture(mesh),

    totalInternalEnergy_
    (
        lookupOrDefault<Switch>("totalInternalEnergy", true)
    ),

    p_
    (
        IOobject
        (
            "p",
            mesh.time().name(),
            mesh,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh
    ),
    T_
    (
        IOobject
        (
            "T",
            mesh.time().name(),
            mesh,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh
    ),

    thermo1_(nullptr),
    thermo2_(nullptr),

    rho_
    (
        IOobject
        (
            "rho",
            mesh.time().name(),
            mesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedScalar("rho", dimDensity, 0)
    ),

    nu_
    (
        IOobject
        (
            "nu",
            mesh.time().name(),
            mesh
        ),
        mesh,
        dimensionedScalar(dimKinematicViscosity, 0),
        calculatedFvPatchScalarField::typeName
    )
{
    {
        volScalarField T1
        (
            IOobject
            (
                IOobject::groupName("T", phase1Name()),
                mesh.time().name(),
                mesh
            ),
            T_,
            calculatedFvPatchScalarField::typeName
        );
        T1.write();
    }

    {
        volScalarField T2
        (
            IOobject
            (
                IOobject::groupName("T", phase2Name()),
                mesh.time().name(),
                mesh
            ),
            T_,
            calculatedFvPatchScalarField::typeName
        );
        T2.write();
    }

    // Note: we're writing files to be read in immediately afterwards.
    //       Avoid any thread-writing problems.
    // fileHandler().flush();

    thermo1_ = rhoFluidThermo::New(mesh, phase1Name());
    thermo2_ = rhoFluidThermo::New(mesh, phase2Name());

    // thermo1_->validate(phase1Name(), "e");
    // thermo2_->validate(phase2Name(), "e");

    correct();
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::compressibleTwoPhaseVoFMixture::~compressibleTwoPhaseVoFMixture()
{}


// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

void Foam::compressibleTwoPhaseVoFMixture::correctThermo()
{
    thermo1_->T() = T_;
    thermo1_->he() = thermo1_->he(p_, T_);
    thermo1_->correct();

    thermo2_->T() = T_;
    thermo2_->he() = thermo2_->he(p_, T_);
    thermo2_->correct();
}


void Foam::compressibleTwoPhaseVoFMixture::correct()
{
    const volScalarField alphaRho1(alpha1()*thermo1_->rho());
    const volScalarField alphaRho2(alpha2()*thermo2_->rho());

    rho_ = alpha1()*thermo1_->rho() + alpha2()*thermo2_->rho();
    nu_ = (alpha1()*thermo1_->mu() + alpha2()*thermo2_->mu())/rho_;
}


Foam::tmp<Foam::volScalarField>
Foam::compressibleTwoPhaseVoFMixture::psiByRho() const
{
    return
    (
        alpha1()*thermo1_->psi()/thermo1_->rho()
      + alpha2()*thermo2_->psi()/thermo2_->rho()
    );
}


bool Foam::compressibleTwoPhaseVoFMixture::read()
{
    if (twoPhaseVoFMixture::read())
    {
        totalInternalEnergy_ =
            lookupOrDefault<Switch>("totalInternalEnergy", true);

        return true;
    }
    else
    {
        return false;
    }
}


// ************************************************************************* //
