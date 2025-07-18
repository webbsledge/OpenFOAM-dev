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
    Foam::fixedJumpFvPatchField

Description
    This boundary condition provides a jump condition, using the \c cyclic
    condition as a base. The jump is specified as the difference between the
    neighbour patch and the owner patch value (i.e., neighbour minus owner).

Usage
    \table
        Property     | Description             | Required    | Default value
        patchType    | underlying patch type (should be \c cyclic) | yes |
        jump         | jump value              | yes         |
    \endtable

    Example of the boundary condition specification:
    \verbatim
    <patchName>
    {
        type            fixedJump;
        patchType       cyclic;
        jump            uniform 10;
    }
    \endverbatim

See also
    Foam::jumpCyclicFvPatchField

SourceFiles
    fixedJumpFvPatchField.C

\*---------------------------------------------------------------------------*/

#ifndef fixedJumpFvPatchField_H
#define fixedJumpFvPatchField_H

#include "jumpCyclicFvPatchField.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                    Class fixedJumpFvPatchField Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class fixedJumpFvPatchField
:
    public jumpCyclicFvPatchField<Type>
{
    // Private data

        //- The "jump" field. Applied to the owner field when transforming
        //  to the neighbour side. Negated when transforming the other way.
        //  Only has a valid value on the owner patch.
        Field<Type> jump_;


protected:

    // Protected functions

        //- Access the jump to modify it
        Field<Type>& jumpRef();


public:

    //- Runtime type information
    TypeName("fixedJump");

    // Constructors

        //- Construct from patch, internal field and dictionary
        fixedJumpFvPatchField
        (
            const fvPatch&,
            const DimensionedField<Type, volMesh>&,
            const dictionary&,
            const bool jumpRequired=true
        );

        //- Construct by mapping given fixedJumpFvPatchField onto a
        //  new patch
        fixedJumpFvPatchField
        (
            const fixedJumpFvPatchField<Type>&,
            const fvPatch&,
            const DimensionedField<Type, volMesh>&,
            const fieldMapper&
        );

        //- Disallow copy without setting internal field reference
        fixedJumpFvPatchField
        (
            const fixedJumpFvPatchField<Type>&
        ) = delete;

        //- Copy constructor setting internal field reference
        fixedJumpFvPatchField
        (
            const fixedJumpFvPatchField<Type>&,
            const DimensionedField<Type, volMesh>&
        );

        //- Construct and return a clone setting internal field reference
        virtual tmp<fvPatchField<Type>> clone
        (
            const DimensionedField<Type, volMesh>& iF
        ) const
        {
            return tmp<fvPatchField<Type>>
            (
                new fixedJumpFvPatchField<Type>(*this, iF)
            );
        }


    // Member Functions

        // Access

            //- Return the "jump"
            virtual tmp<Field<Type>> jump() const;


        // Mapping functions

            //- Map the given fvPatchField onto this fvPatchField
            virtual void map(const fvPatchField<Type>&, const fieldMapper&);

            //- Reset the fvPatchField to the given fvPatchField
            //  Used for mesh to mesh mapping
            virtual void reset(const fvPatchField<Type>&);


        //- Write
        virtual void write(Ostream&) const;


    // Member Operators

        //- Inherit assignment
        using jumpCyclicFvPatchField<Type>::operator=;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "fixedJumpFvPatchField.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
