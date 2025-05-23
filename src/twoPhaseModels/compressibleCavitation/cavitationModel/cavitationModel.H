/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2021-2025 OpenFOAM Foundation
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
    Foam::compressible::twoPhaseChangeModels::cavitationModel

Description
    Abstract base class for cavitation models

SourceFiles
    cavitationModel.C

\*---------------------------------------------------------------------------*/

#ifndef cavitationModel_H
#define cavitationModel_H

#include "compressibleTwoPhases.H"
#include "fvMatricesFwd.H"
#include "Pair.H"
#include "saturationPressureModel.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace compressible
{

/*---------------------------------------------------------------------------*\
                               Class cavitationModel
\*---------------------------------------------------------------------------*/

class cavitationModel
{
protected:

    // Protected data

        //- Phases
        const compressibleTwoPhases& phases_;

        //- Index of the liquid
        const bool liquidIndex_;

        //- The saturation pressure model
        autoPtr<saturationPressureModel> saturationModel_;


    // Protected Member Functions

        //- Return the liquid density
        inline const volScalarField::Internal& alphal() const
        {
            return phases_.alpha(liquidIndex_);
        }

        //- Return the vapour density
        inline const volScalarField::Internal& alphav() const
        {
            return phases_.alpha(!liquidIndex_);
        }

        //- Return the liquid density
        inline const volScalarField::Internal& rhol() const
        {
            return phases_.rho(liquidIndex_);
        }

        //- Return the vapour density
        inline const volScalarField::Internal& rhov() const
        {
            return phases_.rho(!liquidIndex_);
        }

        //- Return the liquid thermo
        inline const rhoFluidThermo& thermol() const
        {
            return phases_.thermo(liquidIndex_);
        }

        //- Return the vapour thermo
        inline const rhoFluidThermo& thermov() const
        {
            return phases_.thermo(!liquidIndex_);
        }

        //- Return the saturation vapour pressure for the liquid
        inline tmp<volScalarField::Internal> pSatl() const
        {
            return saturationModel_->pSat(thermol().T()());
        }

        //- Return the saturation vapour pressure for the vapour
        inline tmp<volScalarField::Internal> pSatv() const
        {
            return saturationModel_->pSat(thermov().T()());
        }


public:

    //- Runtime type information
    TypeName("cavitation");


    // Declare run-time constructor selection table

        declareRunTimeSelectionTable
        (
            autoPtr,
            cavitationModel,
            dictionary,
            (
                const dictionary& dict,
                const compressibleTwoPhases& phases,
                const label liquidIndex
            ),
            (dict, phases, liquidIndex)
        );


    // Constructors

        //- Construct for phases
        cavitationModel
        (
            const dictionary& dict,
            const compressibleTwoPhases& phases,
            const label liquidIndex
        );


    // Selector
    static autoPtr<cavitationModel> New
    (
        const dictionary& dict,
        const compressibleTwoPhases& phases,
        const label liquidIndex = -1
    );


    //- Destructor
    virtual ~cavitationModel()
    {}


    // Member Functions

        //- Return the saturation vapour pressure for phase 1
        inline tmp<volScalarField::Internal> pSat1() const
        {
            return liquidIndex_ ? pSatv() : pSatl();
        }

        //- Return the saturation vapour pressure for phase 2
        inline tmp<volScalarField::Internal> pSat2() const
        {
            return liquidIndex_ ? pSatl() : pSatv();
        }

        //- Return the mass condensation and vaporisation rates as a
        //  coefficient to multiply alphav for the condensation rate and a
        //  coefficient to multiply alphal for the vaporisation rate
        virtual Pair<tmp<volScalarField::Internal>> mDotcvAlphal() const = 0;

        //- Return the mass condensation and vaporisation rates as coefficients
        //  to multiply (p - pSat)
        virtual Pair<tmp<volScalarField::Internal>> mDotcvP() const = 0;

        //- Return the mass transfer rates of the two phases as coefficients to
        //  multiply the volume fraction of the other phase
        inline Pair<tmp<volScalarField::Internal>> mDot12Alpha() const
        {
            return liquidIndex_ ? reverse(mDotcvAlphal()) : mDotcvAlphal();
        }

        //- Return the mass transfer rates of the two phases as coefficients to
        //  multiply (p - pSat)
        inline Pair<tmp<volScalarField::Internal>> mDot12P() const
        {
            return liquidIndex_ ? reverse(mDotcvP()) : mDotcvP();
        }

        //- Correct the cavitation model
        virtual void correct() = 0;

        //- Read the dictionary and update
        virtual bool read(const dictionary& dict) = 0;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace compressible
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
