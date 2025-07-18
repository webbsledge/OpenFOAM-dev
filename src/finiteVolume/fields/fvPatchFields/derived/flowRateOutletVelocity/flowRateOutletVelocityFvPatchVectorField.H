/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2017-2025 OpenFOAM Foundation
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
    Foam::flowRateOutletVelocityFvPatchVectorField

Description
    Velocity outlet boundary condition which corrects the extrapolated velocity
    to match the specified flow rate.

    For a mass-based flux:
    - the flow rate should be provided in kg/s
    - if \c rho is "none" the flow rate is in m^3/s
    - otherwise \c rho should correspond to the name of the density field
    - if the density field cannot be found in the database, the user must
      specify the outlet density using the \c rhoOutlet entry

    For a volumetric-based flux:
    - the flow rate is in m^3/s

Usage
    \table
        Property     | Description             | Required    | Default value
        massFlowRate | Mass flow rate [kg/s]   | no          |
        volumetricFlowRate | Volumetric flow rate [m^3/s] | no |
        rho          | Density field name      | no          | rho
        rhoOutlet    | Outlet density          | no          |
    \endtable

    Example of the boundary condition specification for a volumetric flow rate:
    \verbatim
    <patchName>
    {
        type                flowRateOutletVelocity;
        volumetricFlowRate  0.2;
        value               uniform (0 0 0);
    }
    \endverbatim

    Example of the boundary condition specification for a mass flow rate:
    \verbatim
    <patchName>
    {
        type                flowRateOutletVelocity;
        massFlowRate        0.2;
        rhoOutlet           1.0;
        value               uniform (0 0 0);
    }
    \endverbatim

    The c volumetricFlowRate or \c massFlowRate entry is a \c Function1 of
    time, see Foam::Function1s.

    Note:
      - \c rhoOutlet is required for the case of a mass flow rate, where the
        density field is not available at start-up
      - The value is positive out of the domain (as an outlet)
      - May not work correctly for transonic outlets
      - Strange behaviour with potentialFoam since the U equation is not solved

See also
    Foam::fixedValueFvPatchField
    Foam::Function1s
    Foam::flowRateInletVelocityFvPatchVectorField

SourceFiles
    flowRateOutletVelocityFvPatchVectorField.C

\*---------------------------------------------------------------------------*/

#ifndef flowRateOutletVelocityFvPatchVectorField_H
#define flowRateOutletVelocityFvPatchVectorField_H

#include "fixedValueFvPatchFields.H"
#include "Function1.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
          Class flowRateOutletVelocityFvPatchVectorField Declaration
\*---------------------------------------------------------------------------*/

class flowRateOutletVelocityFvPatchVectorField
:
    public fixedValueFvPatchVectorField
{
    // Private Data

        //- Outlet integral flow rate
        autoPtr<Function1<scalar>> flowRate_;

        //- Is the flow-rate volumetric?
        bool volumetric_;

        //- Name of the density field used to normalise the mass flux
        word rhoName_;

        //- Rho initialisation value (for start; if value not supplied)
        scalar rhoOutlet_;


    // Private Member Functions

        //- Update the patch values given the appropriate density type and value
        template<class RhoType>
        void updateValues(const RhoType& rho);


public:

   //- Runtime type information
   TypeName("flowRateOutletVelocity");


   // Constructors

        //- Construct from patch, internal field and dictionary
        flowRateOutletVelocityFvPatchVectorField
        (
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping given
        //  flowRateOutletVelocityFvPatchVectorField
        //  onto a new patch
        flowRateOutletVelocityFvPatchVectorField
        (
            const flowRateOutletVelocityFvPatchVectorField&,
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const fieldMapper&
        );

        //- Disallow copy without setting internal field reference
        flowRateOutletVelocityFvPatchVectorField
        (
            const flowRateOutletVelocityFvPatchVectorField&
        ) = delete;

        //- Copy constructor setting internal field reference
        flowRateOutletVelocityFvPatchVectorField
        (
            const flowRateOutletVelocityFvPatchVectorField&,
            const DimensionedField<vector, volMesh>&
        );

        //- Construct and return a clone setting internal field reference
        virtual tmp<fvPatchVectorField> clone
        (
            const DimensionedField<vector, volMesh>& iF
        ) const
        {
            return tmp<fvPatchVectorField>
            (
                new flowRateOutletVelocityFvPatchVectorField(*this, iF)
            );
        }


    // Member Functions

        //- Update the coefficients associated with the patch field
        virtual void updateCoeffs();

        //- Write
        virtual void write(Ostream&) const;


    // Member Operators

        //- Inherit assignment
        using fixedValueFvPatchVectorField::operator=;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
