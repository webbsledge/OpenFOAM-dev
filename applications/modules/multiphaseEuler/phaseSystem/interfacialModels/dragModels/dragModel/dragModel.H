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
    Foam::dragModel

Description
    Model for drag between phases

SourceFiles
    dragModel.C
    dragModelNew.C

\*---------------------------------------------------------------------------*/

#ifndef dragModel_H
#define dragModel_H

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "volFields.H"
#include "dictionary.H"
#include "runTimeSelectionTables.H"
#include "BlendedInterfacialModel.H"

namespace Foam
{

/*---------------------------------------------------------------------------*\
                          Class dragModel Declaration
\*---------------------------------------------------------------------------*/

class dragModel
:
    public regIOobject
{
public:

    //- Runtime type information
    TypeName("dragModel");


    // Declare runtime construction

        declareRunTimeSelectionTable
        (
            autoPtr,
            dragModel,
            dictionary,
            (
                const dictionary& dict,
                const phaseInterface& interface,
                const bool registerObject
            ),
            (dict, interface, registerObject)
        );


    // Static Data Members

        //- Coefficient dimensions
        static const dimensionSet dimK;


    // Constructors

        // Construct from a dictionary and an interface
        dragModel
        (
            const dictionary& dict,
            const phaseInterface& interface,
            const bool registerObject
        );


    //- Destructor
    virtual ~dragModel();


    // Selectors

        static autoPtr<dragModel> New
        (
            const dictionary& dict,
            const phaseInterface& interface,
            const bool outer=true,
            const bool registerObject=true
        );


    // Member Functions

        //- Return the drag coefficient K
        //  used in the momentum equations
        //    ddt(alpha1*rho1*U1) + ... = ... K*(U1-U2)
        //    ddt(alpha2*rho2*U2) + ... = ... K*(U2-U1)
        virtual tmp<volScalarField> K() const = 0;

        //- Return the drag coefficient Kf
        //  used in the face-momentum equations
        virtual tmp<surfaceScalarField> Kf() const = 0;

        //- Dummy write for regIOobject
        bool writeData(Ostream& os) const;
};


/*---------------------------------------------------------------------------*\
                      Class blendedDragModel Declaration
\*---------------------------------------------------------------------------*/

class blendedDragModel
:
    public BlendedInterfacialModel<dragModel>
{
public:

    // Constructors

        //- Inherit base class constructors
        using BlendedInterfacialModel<dragModel>::BlendedInterfacialModel;


    // Selectors

        inline static autoPtr<blendedDragModel> New
        (
            const dictionary& dict,
            const phaseInterface& interface,
            const dictionary& blendingDict
        )
        {
            return autoPtr<blendedDragModel>
            (
                new blendedDragModel(dict, interface, blendingDict)
            );
        }


    // Member Functions

        //- Return the drag coefficient K
        tmp<volScalarField> K() const;

        //- Return the drag coefficient Kf
        tmp<surfaceScalarField> Kf() const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
