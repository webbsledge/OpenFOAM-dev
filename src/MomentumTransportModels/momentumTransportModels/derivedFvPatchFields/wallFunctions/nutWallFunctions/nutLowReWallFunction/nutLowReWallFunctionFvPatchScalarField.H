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

Class
    Foam::nutLowReWallFunctionFvPatchScalarField

Description
    This boundary condition provides a turbulent kinematic viscosity condition
    for use with low Reynolds number models.  It sets \c nut to zero, and
    provides an access function to calculate y+.

Usage
    Example of the boundary condition specification:
    \verbatim
    <patchName>
    {
        type            nutLowReWallFunction;
        value           uniform 0;
    }
    \endverbatim

See also
    Foam::nutWallFunctionFvPatchScalarField

SourceFiles
    nutLowReWallFunctionFvPatchScalarField.C

\*---------------------------------------------------------------------------*/

#ifndef nutLowReWallFunctionFvPatchScalarField_H
#define nutLowReWallFunctionFvPatchScalarField_H

#include "nutWallFunctionFvPatchScalarField.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
           Class nutLowReWallFunctionFvPatchScalarField Declaration
\*---------------------------------------------------------------------------*/

class nutLowReWallFunctionFvPatchScalarField
:
    public nutWallFunctionFvPatchScalarField
{
protected:

    // Protected Member Functions

        //- Calculate the turbulence viscosity
        virtual tmp<scalarField> nut() const;


public:

    //- Runtime type information
    TypeName("nutLowReWallFunction");


    // Constructors

        //- Construct from patch, internal field and dictionary
        nutLowReWallFunctionFvPatchScalarField
        (
            const fvPatch&,
            const DimensionedField<scalar, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping given
        //  nutLowReWallFunctionFvPatchScalarField
        //  onto a new patch
        nutLowReWallFunctionFvPatchScalarField
        (
            const nutLowReWallFunctionFvPatchScalarField&,
            const fvPatch&,
            const DimensionedField<scalar, volMesh>&,
            const fieldMapper&
        );

        //- Disallow copy without setting internal field reference
        nutLowReWallFunctionFvPatchScalarField
        (
            const nutLowReWallFunctionFvPatchScalarField&
        ) = delete;

        //- Copy constructor setting internal field reference
        nutLowReWallFunctionFvPatchScalarField
        (
            const nutLowReWallFunctionFvPatchScalarField&,
            const DimensionedField<scalar, volMesh>&
        );

        //- Construct and return a clone setting internal field reference
        virtual tmp<fvPatchScalarField> clone
        (
            const DimensionedField<scalar, volMesh>& iF
        ) const
        {
            return tmp<fvPatchScalarField>
            (
                new nutLowReWallFunctionFvPatchScalarField(*this, iF)
            );
        }


    // Member Functions

        //- Calculate and return the yPlus at the boundary
        virtual tmp<scalarField> yPlus() const;


    // Member Operators

        //- Inherit assignment
        using nutWallFunctionFvPatchScalarField::operator=;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
