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
    Foam::rotatingWallVelocityFvPatchVectorField

Description
    Condition on velocity for a boundary consisting of a rotating solid of
    revolution, e.g. cylinder. Calculates a tangential component of velocity
    from the angular velocity and rotational axis and ensures a zero normal
    component.

Usage
    \table
        Property     | Description             | Required    | Default value
        origin       | origin of rotation in Cartesian co-ordinates | yes|
        axis         | axis of rotation        | yes         |
        omega        | angular velocity of the frame | no |
    \endtable

    Example of the boundary condition specification:
    \verbatim
    <patchName>
    {
        type            rotatingWallVelocity;
        origin          (0 0 0);
        axis            (0 0 1);
        omega           100 [rpm];
    }
    \endverbatim

    The \c omega entry is a Function1 of time, see Foam::Function1s.

See also
    Foam::fixedValueFvPatchField
    Foam::Function1s

SourceFiles
    rotatingWallVelocityFvPatchVectorField.C

\*---------------------------------------------------------------------------*/

#ifndef rotatingWallVelocityFvPatchVectorField_H
#define rotatingWallVelocityFvPatchVectorField_H

#include "fixedValueFvPatchFields.H"
#include "omega1.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
           Class rotatingWallVelocityFvPatchVectorField Declaration
\*---------------------------------------------------------------------------*/

class rotatingWallVelocityFvPatchVectorField
:
    public fixedValueFvPatchVectorField
{
    // Private Data

        //- Origin of the rotation
        vector origin_;

        //- Axis of the rotation
        vector axis_;

        //- Rotational speed
        Function1s::omega omega_;


public:

    //- Runtime type information
    TypeName("rotatingWallVelocity");


    // Constructors

        //- Construct from patch, internal field and dictionary
        rotatingWallVelocityFvPatchVectorField
        (
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping given rotatingWallVelocityFvPatchVectorField
        //  onto a new patch
        rotatingWallVelocityFvPatchVectorField
        (
            const rotatingWallVelocityFvPatchVectorField&,
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const fieldMapper&
        );

        //- Disallow copy without setting internal field reference
        rotatingWallVelocityFvPatchVectorField
        (
            const rotatingWallVelocityFvPatchVectorField&
        ) = delete;

        //- Copy constructor setting internal field reference
        rotatingWallVelocityFvPatchVectorField
        (
            const rotatingWallVelocityFvPatchVectorField&,
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
                new rotatingWallVelocityFvPatchVectorField(*this, iF)
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
