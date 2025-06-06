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

#include "IATE.H"
#include "IATEsource.H"
#include "fvmDdt.H"
#include "fvmDiv.H"
#include "fvmSup.H"
#include "fvcDdt.H"
#include "fvcDiv.H"
#include "fvcAverage.H"
#include "fvModels.H"
#include "fvConstraints.H"
#include "mathematicalConstants.H"
#include "fundamentalConstants.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace diameterModels
{
    defineTypeNameAndDebug(IATE, 0);
    addToRunTimeSelectionTable(diameterModel, IATE, dictionary);
}
}


// * * * * * * * * * * * * Private Member Functions * * * * * * * * * * * * //

Foam::tmp<Foam::volScalarField> Foam::diameterModels::IATE::dsm() const
{
    return max(6/max(kappai_, 6/dMax_), dMin_);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::diameterModels::IATE::IATE
(
    const dictionary& diameterProperties,
    const phaseModel& phase
)
:
    diameterModel(diameterProperties, phase),
    kappai_
    (
        IOobject
        (
            IOobject::groupName("kappai", phase.name()),
            phase.time().name(),
            phase.mesh(),
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        phase.mesh()
    ),
    dMax_("dMax", dimLength, diameterProperties),
    dMin_("dMin", dimLength, diameterProperties),
    residualAlpha_("residualAlpha", dimless, diameterProperties),
    d_
    (
        IOobject
        (
            IOobject::groupName("d", phase.name()),
            phase.time().name(),
            phase.mesh(),
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        dsm()
    ),
    sources_(diameterProperties.lookup("sources"), IATEsource::iNew(*this))
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::diameterModels::IATE::~IATE()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::tmp<Foam::volScalarField> Foam::diameterModels::IATE::d() const
{
    return d_;
}


Foam::tmp<Foam::volScalarField> Foam::diameterModels::IATE::Av() const
{
    return phase()*kappai_;
}


void Foam::diameterModels::IATE::correct()
{
    const volScalarField& alpha = phase();
    const volScalarField& rho = phase().rho();

    volScalarField alphaAv
    (
        max
        (
            0.5*fvc::average(alpha + alpha.oldTime()),
            residualAlpha_
        )
    );

    // Initialise the accumulated source term to the dilatation effect
    fvScalarMatrix R
    (
       -fvm::SuSp
        (
            ((1.0/3.0)/alphaAv)
           *(
                (fvc::ddt(alpha) + fvc::div(phase().alphaPhi()))
              - (fvc::ddt(alpha, rho) + fvc::div(phase().alphaRhoPhi()))/rho
            ),
            kappai_
        )
    );

    // Accumulate the run-time selectable sources
    forAll(sources_, j)
    {
        R += sources_[j].R(kappai_);
    }

    const Foam::fvModels& fvModels =
        Foam::fvModels::New(phase().mesh());
    const Foam::fvConstraints& fvConstraints =
        Foam::fvConstraints::New(phase().mesh());

    // Construct the interfacial curvature equation
    fvScalarMatrix kappaiEqn
    (
        fvm::ddt(kappai_) + fvm::div(phase().phi(), kappai_)
      - fvm::Sp(fvc::div(phase().phi()), kappai_)
     ==
        R
      + fvModels.source(alpha, rho, kappai_)/(alphaAv*rho)
    );

    kappaiEqn.relax();

    fvConstraints.constrain(kappaiEqn);

    kappaiEqn.solve();

    // Update the Sauter-mean diameter
    d_ = dsm();
}


bool Foam::diameterModels::IATE::read(const dictionary& phaseProperties)
{
    diameterModel::read(phaseProperties);

    diameterProperties().lookup("dMax") >> dMax_;
    diameterProperties().lookup("dMin") >> dMin_;

    diameterProperties().lookup("residualAlpha") >> residualAlpha_;

    // Re-create all the sources updating number, type and coefficients
    PtrList<IATEsource>
    (
        diameterProperties().lookup("sources"),
        IATEsource::iNew(*this)
    ).transfer(sources_);

    return true;
}


// ************************************************************************* //
