/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2023-2025 OpenFOAM Foundation
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

#include "solidDisplacement.H"
#include "fvcGrad.H"
#include "fvcDiv.H"
#include "fvcLaplacian.H"
#include "fvmD2dt2.H"
#include "fvmLaplacian.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace solvers
{
    defineTypeNameAndDebug(solidDisplacement, 0);
    addToRunTimeSelectionTable(solver, solidDisplacement, fvMesh);
}
}


// * * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * //

bool Foam::solvers::solidDisplacement::dependenciesModified() const
{
    return solid::dependenciesModified() || mesh.solution().modified();
}


bool Foam::solvers::solidDisplacement::read()
{
    solid::read();

    nCorr = pimple.dict().lookupOrDefault<int>("nCorrectors", 1);
    convergenceTolerance = pimple.dict().lookupOrDefault<scalar>("D", 0);
    pimple.dict().lookup("compactNormalStress") >> compactNormalStress;
    accFac = pimple.dict().lookupOrDefault<scalar>("accelerationFactor", 1);

    return true;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::solvers::solidDisplacement::solidDisplacement(fvMesh& mesh)
:
    solid
    (
        mesh,
        autoPtr<solidThermo>(new solidDisplacementThermo(mesh))
    ),

    thermo_(refCast<solidDisplacementThermo>(solid::thermo_)),

    compactNormalStress(pimple.dict().lookup("compactNormalStress")),

    D_
    (
        IOobject
        (
            "D",
            runTime.name(),
            mesh,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh
    ),

    E(thermo_.E()),
    nu(thermo_.nu()),

    mu(E/(2*(1 + nu))),

    lambda
    (
        thermo_.planeStress()
      ? nu*E/((1 + nu)*(1 - nu))
      : nu*E/((1 + nu)*(1 - 2*nu))
    ),

    threeK
    (
        thermo_.planeStress()
      ? E/(1 - nu)
      : E/(1 - 2*nu)
    ),

    threeKalpha("threeKalpha", threeK*thermo_.alphav()),

    sigmaD
    (
        IOobject
        (
            "sigmaD",
            runTime.name(),
            mesh
        ),
        mu*twoSymm(fvc::grad(D_)) + lambda*(I*tr(fvc::grad(D_)))
    ),

    divSigmaExp
    (
        IOobject
        (
            "divSigmaExp",
            runTime.name(),
            mesh
        ),
        fvc::div(sigmaD)
      - (
            compactNormalStress
          ? fvc::laplacian(2*mu + lambda, D_, "laplacian(DD,D)")
          : fvc::div((2*mu + lambda)*fvc::grad(D_), "div(sigmaD)")
        )
    ),

    thermo(thermo_),
    D(D_)
{
    mesh.schemes().setFluxRequired(D.name());

    // Read the controls
    read();
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::solvers::solidDisplacement::~solidDisplacement()
{}


// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

void Foam::solvers::solidDisplacement::prePredictor()
{
    if (thermo.thermalStress())
    {
        solid::prePredictor();
    }
}


void Foam::solvers::solidDisplacement::thermophysicalPredictor()
{
    if (thermo.thermalStress())
    {
        solid::thermophysicalPredictor();
    }
}


void Foam::solvers::solidDisplacement::pressureCorrector()
{
    volVectorField& D(D_);

    const volScalarField& rho = thermo_.rho();

    int iCorr = 0;
    scalar initialResidual = 0;

    {
        {
            fvVectorMatrix DEqn
            (
                fvm::d2dt2(rho, D)
             ==
                fvm::laplacian(2*mu + lambda, D, "laplacian(DD,D)")
              + divSigmaExp
              + rho*fvModels().d2dt2(D)
            );

            if (thermo.thermalStress())
            {
                DEqn += fvc::grad(threeKalpha*T);
            }

            fvConstraints().constrain(DEqn);

            initialResidual = DEqn.solve().max().initialResidual();

            // For steady-state optionally accelerate the solution
            // by over-relaxing the displacement
            if (mesh.schemes().steady() && accFac > 1)
            {
                D += (accFac - 1)*(D - D.oldTime());
            }

            if (!compactNormalStress)
            {
                divSigmaExp = fvc::div(DEqn.flux());
            }
        }

        const volTensorField gradD(fvc::grad(D));
        sigmaD = mu*twoSymm(gradD) + (lambda*I)*tr(gradD);

        if (compactNormalStress)
        {
            divSigmaExp = fvc::div
            (
                sigmaD - (2*mu + lambda)*gradD,
                "div(sigmaD)"
            );
        }
        else
        {
            divSigmaExp += fvc::div(sigmaD);
        }

    } while (initialResidual > convergenceTolerance && ++iCorr < nCorr);
}


void Foam::solvers::solidDisplacement::thermophysicalTransportCorrector()
{
    if (thermo.thermalStress())
    {
        solid::thermophysicalTransportCorrector();
    }
}


void Foam::solvers::solidDisplacement::postSolve()
{
    if (runTime.writeTime())
    {
        volSymmTensorField sigma
        (
            IOobject
            (
                "sigma",
                runTime.name(),
                mesh
            ),
            sigmaD
        );

        if (thermo.thermalStress())
        {
            sigma = sigma - I*(threeKalpha*thermo.T());
        }

        volScalarField sigmaEq
        (
            IOobject
            (
                "sigmaEq",
                runTime.name(),
                mesh
            ),
            sqrt((3.0/2.0)*magSqr(dev(sigma)))
        );

        Info<< "Max sigmaEq = " << max(sigmaEq).value()
            << endl;

        sigma.write();
        sigmaEq.write();
    }
}


// ************************************************************************* //
