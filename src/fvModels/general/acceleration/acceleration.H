/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2018-2025 OpenFOAM Foundation
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
    Foam::fv::acceleration

Description
    This fvModel applies an explicit acceleration force to components of the
    velocity field.

Usage
    Example usage:
    \verbatim
    acceleration1
    {
        type            acceleration;

        cellZone        all;

        U               U;

        velocity        scale;
        value           (-2.572 0 0);
        scale
        {
            type            halfCosineRamp;
            start           0;
            duration        10;
        }
    }
    \endverbatim

SourceFiles
    acceleration.C

\*---------------------------------------------------------------------------*/

#ifndef acceleration_H
#define acceleration_H

#include "fvModel.H"
#include "fvCellZone.H"
#include "Function1.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{

/*---------------------------------------------------------------------------*\
                        Class acceleration Declaration
\*---------------------------------------------------------------------------*/

class acceleration
:
    public fvModel
{
    // Private Data

        //- The cellZone the fvConstraint applies to
        fvCellZone zone_;

        //- Name of the velocity field
        word UName_;

        //- Time-varying velocity
        autoPtr<Function1<vector>> velocity_;


    // Private Member Functions

        //- Non-virtual read
        void readCoeffs(const dictionary& dict);

        //- Source term to momentum equation
        template<class AlphaRhoFieldType>
        inline void add
        (
            const AlphaRhoFieldType& rho,
            fvMatrix<vector>& eqn
        ) const;


public:

    //- Runtime type information
    TypeName("acceleration");


    // Constructors

        //- Construct from components
        acceleration
        (
            const word& name,
            const word& modelType,
            const fvMesh& mesh,
            const dictionary& dict
        );


    //- Destructor
    virtual ~acceleration()
    {}


    // Member Functions

        // Checks

            //- Return the list of fields for which the fvModel adds source term
            //  to the transport equation
            virtual wordList addSupFields() const;


         // Add explicit and implicit contributions

            //- Source term to momentum equation
            virtual void addSup
            (
                const volVectorField& U,
                fvMatrix<vector>& eqn
            ) const;

            //- Source term to compressible momentum equation
            virtual void addSup
            (
                const volScalarField& rho,
                const volVectorField& U,
                fvMatrix<vector>& eqn
            ) const;

            //- Source term to phase momentum equation
            virtual void addSup
            (
                const volScalarField& alpha,
                const volScalarField& rho,
                const volVectorField& U,
                fvMatrix<vector>& eqn
            ) const;


        // Mesh changes

            //- Update for mesh motion
            virtual bool movePoints();

            //- Update topology using the given map
            virtual void topoChange(const polyTopoChangeMap&);

            //- Update from another mesh using the given map
            virtual void mapMesh(const polyMeshMap&);

            //- Redistribute or update using the given distribution map
            virtual void distribute(const polyDistributionMap&);


        // IO

            //- Read dictionary
            virtual bool read(const dictionary& dict);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
