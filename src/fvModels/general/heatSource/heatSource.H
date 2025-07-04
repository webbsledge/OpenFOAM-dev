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
    Foam::fv::heatSource

Description
    Model for applying a heat source. Requires either the power, Q, or the
    power per unit volume, q, to be specified.

Usage
    Example usage:
    \verbatim
    heatSource
    {
        type            heatSource;

        cellZone        heater;

        Q               1e6;
    }
    \endverbatim

\*---------------------------------------------------------------------------*/

#ifndef heatSource_H
#define heatSource_H

#include "fvModel.H"
#include "fvCellZone.H"
#include "Function1.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{

/*---------------------------------------------------------------------------*\
                Class heatSource Declaration
\*---------------------------------------------------------------------------*/

class heatSource
:
    public fvModel
{
    // Private data

        //- The cellZone the model applies to
        fvCellZone zone_;

        //- The heat source
        autoPtr<Function1<scalar>> q_;


    // Private member functions

        //- Non-virtual read
        void readCoeffs(const dictionary& dict);


public:

    //- Runtime type information
    TypeName("heatSource");


    // Constructors

        //- Construct from dictionary
        heatSource
        (
            const word& name,
            const word& modelType,
            const fvMesh& mesh,
            const dictionary& dict
        );


    //- Destructor
    virtual ~heatSource();


    // Member Functions

        // Checks

            //- Return the list of fields for which the fvModel adds source term
            //  to the transport equation
            virtual wordList addSupFields() const;


        // Sources

            //- Source term to energy equation
            virtual void addSup
            (
                const volScalarField& he,
                fvMatrix<scalar>& eqn
            ) const;

            //- Source term to compressible energy equation
            virtual void addSup
            (
                const volScalarField& rho,
                const volScalarField& he,
                fvMatrix<scalar>& eqn
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
