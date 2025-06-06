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
    Foam::dragModels::segregated

Description
    Segregated drag model for use in regions with no obvious dispersed phase.

    Reference:
    \verbatim
        Marschall, H. (2011).
        Towards the numerical simulation of multi-scale two-phase flows.
        PhD Thesis, TU München.
    \endverbatim

SourceFiles
    segregated.C

\*---------------------------------------------------------------------------*/

#ifndef segregated_H
#define segregated_H

#include "dragModel.H"
#include "segregatedPhaseInterface.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace dragModels
{

/*---------------------------------------------------------------------------*\
                         Class segregated Declaration
\*---------------------------------------------------------------------------*/

class segregated
:
    public dragModel
{
    // Private Data

        //- Interface
        const segregatedPhaseInterface interface_;

        //- M coefficient
        const dimensionedScalar m_;

        //- N coefficient
        const dimensionedScalar n_;


public:

    //- Runtime type information
    TypeName("segregated");


    // Constructors

        //- Construct from a dictionary and an interface
        segregated
        (
            const dictionary& dict,
            const phaseInterface& interface,
            const bool registerObject
        );


    //- Destructor
    virtual ~segregated();


    // Member Functions

        //- The drag function used in the momentum equation
        virtual tmp<volScalarField> K() const;

        //- The drag function Kf used in the face-momentum equations
        virtual tmp<surfaceScalarField> Kf() const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace dragModels
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
