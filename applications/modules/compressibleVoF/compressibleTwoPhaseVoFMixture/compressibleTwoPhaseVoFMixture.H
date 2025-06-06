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

Class
    Foam::compressibleTwoPhaseVoFMixture

Description
    Class to represent a mixture of two rhoFluidThermo-based phases

SourceFiles
    compressibleTwoPhaseVoFMixture.C

\*---------------------------------------------------------------------------*/

#ifndef compressibleTwoPhaseVoFMixture_H
#define compressibleTwoPhaseVoFMixture_H

#include "twoPhaseVoFMixture.H"
#include "compressibleTwoPhases.H"
#include "rhoFluidThermo.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                    Class compressibleTwoPhaseVoFMixture Declaration
\*---------------------------------------------------------------------------*/

class compressibleTwoPhaseVoFMixture
:
    public twoPhaseVoFMixture,
    virtual public compressibleTwoPhases,
    public viscosity
{
    // Private Data

        //- Switch to choose between solving for internal energy
        //  or total internal energy which is the default
        Switch totalInternalEnergy_;

        //- Pressure
        volScalarField p_;

        //- Mixture temperature
        volScalarField T_;

        //- Thermo-package of phase 1
        autoPtr<rhoFluidThermo> thermo1_;

        //- Thermo-package of phase 2
        autoPtr<rhoFluidThermo> thermo2_;

        //- Mixture density
        volScalarField rho_;

        //- Mixture viscosity
        volScalarField nu_;


public:

    //- Runtime type information
    TypeName("compressibleTwoPhaseVoFMixture");


    // Constructors

        //- Construct from a mesh
        compressibleTwoPhaseVoFMixture(const fvMesh& mesh);


    //- Destructor
    virtual ~compressibleTwoPhaseVoFMixture();


    // Member Functions

        //- Return true to solve for total internal energy
        //  return false to solve for internal energy
        bool totalInternalEnergy() const
        {
            return totalInternalEnergy_;
        }

        //- Return pressure [Pa]
        volScalarField& p()
        {
            return p_;
        }

        //- Return mixture temperature [K]
        volScalarField& T()
        {
            return T_;
        }

        //- Return mixture temperature [K]
        const volScalarField& T() const
        {
            return T_;
        }

        //- Return the thermo for phase 1
        const rhoFluidThermo& thermo1() const
        {
            return thermo1_();
        }

        //- Return the thermo for phase 2
        const rhoFluidThermo& thermo2() const
        {
            return thermo2_();
        }

        //- Return the thermo for phase 1
        rhoFluidThermo& thermo1()
        {
            return thermo1_();
        }

        //- Return the thermo for phase 2
        rhoFluidThermo& thermo2()
        {
            return thermo2_();
        }

        //- Return the density of phase 1
        const volScalarField& rho1() const
        {
            return thermo1_->rho();
        }

        //- Return the density of phase 2
        const volScalarField& rho2() const
        {
            return thermo2_->rho();
        }

        //- The fluid is incompressible if both phases are incompressible
        bool incompressible() const
        {
            return
                thermo1_->incompressible()
             && thermo2_->incompressible();
        }

        //- Return mixture density [kg/m^3]
        virtual const volScalarField& rho() const
        {
            return rho_;
        }

        //- Return the mixture compressibility/density
        virtual tmp<volScalarField> psiByRho() const;

        //- Correct the thermodynamics of each phase
        virtual void correctThermo();

        //- Update mixture properties
        virtual void correct();

        //- Kinematic viscosity of mixture [m^2/s]
        virtual tmp<volScalarField> nu() const
        {
            return nu_;
        }

        //- Kinematic viscosity of mixture for patch [m^2/s]
        virtual tmp<scalarField> nu(const label patchi) const
        {
            return nu_.boundaryField()[patchi];
        }


    // IO

        //- Read base phaseProperties dictionary
        virtual bool read();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
