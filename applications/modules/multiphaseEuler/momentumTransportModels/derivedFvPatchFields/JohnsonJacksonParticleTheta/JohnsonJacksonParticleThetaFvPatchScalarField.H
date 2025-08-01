/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2014-2025 OpenFOAM Foundation
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
    Foam::JohnsonJacksonParticleThetaFvPatchScalarField

Description
    Robin condition for the particulate granular temperature.

    References:
    \verbatim
        Reuge, N., Cadoret, L., Coufort-Saudejaud, C., Pannala, S., Syamlal, M.,
        & Caussat, B. (2008).
        Multifluid Eulerian modeling of dense gas–solids fluidised bed
        hydrodynamics: influence of the dissipation parameters.
        Chemical Engineering Science, 63(22), 5540-5551.
    \endverbatim

    \verbatim
        Johnson, P. C., & Jackson, R. (1987).
        Frictional–collisional constitutive relations for granular materials,
        with application to plane shearing.
        Journal of fluid Mechanics, 176, 67-93.
    \endverbatim

SourceFiles
    JohnsonJacksonParticleThetaFvPatchScalarField.C

\*---------------------------------------------------------------------------*/

#ifndef JohnsonJacksonParticleThetaFvPatchScalarField_H
#define JohnsonJacksonParticleThetaFvPatchScalarField_H

#include "mixedFvPatchFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
        Class JohnsonJacksonParticleThetaFvPatchScalarField Declaration
\*---------------------------------------------------------------------------*/

class JohnsonJacksonParticleThetaFvPatchScalarField
:
    public mixedFvPatchScalarField
{
    // Private Data

        //- Particle-wall restitution coefficient
        const scalar restitutionCoefficient_;

        //- Specularity coefficient
        const scalar specularityCoefficient_;


public:

    //- Runtime type information
    TypeName("JohnsonJacksonParticleTheta");


    // Constructors

        //- Construct from patch, internal field and dictionary
        JohnsonJacksonParticleThetaFvPatchScalarField
        (
            const fvPatch&,
            const DimensionedField<scalar, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping onto a new patch
        JohnsonJacksonParticleThetaFvPatchScalarField
        (
            const JohnsonJacksonParticleThetaFvPatchScalarField&,
            const fvPatch&,
            const DimensionedField<scalar, volMesh>&,
            const fieldMapper&
        );

        //- Disallow copy without setting internal field reference
        JohnsonJacksonParticleThetaFvPatchScalarField
        (
            const JohnsonJacksonParticleThetaFvPatchScalarField&
        ) = delete;

        //- Copy constructor setting internal field reference
        JohnsonJacksonParticleThetaFvPatchScalarField
        (
            const JohnsonJacksonParticleThetaFvPatchScalarField&,
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
                new JohnsonJacksonParticleThetaFvPatchScalarField(*this, iF)
            );
        }


    // Member Functions

        //- Update the coefficients
        virtual void updateCoeffs();

        //- Write
        virtual void write(Ostream&) const;


    // Member Operators

        //- Inherit assignment
        using mixedFvPatchScalarField::operator=;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
