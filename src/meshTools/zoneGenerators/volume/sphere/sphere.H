/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2025 OpenFOAM Foundation
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
    Foam::zoneGenerators::sphere

Description
    A zoneGenerator which selects points, cells or faces with centres either
    inside or outside a sphere.

    By default all the points, cells or faces of the mesh are tested for being
    inside or outside the sphere but an optional single zone or list of zones
    may be provided and those points, cells or faces are tested instead.  This
    provides an efficient method of hierarchical sub-division of space where an
    initial selection of points cells or faces is refined by selecting the
    sub-set inside or outside the given sphere rather than having to generate
    another zone to intersect with.

Usage
    \table
        Property     | Description             | Required  | Default value
        type         | Type: sphere            | yes       |
        name         | Name of the zone        | no        | zoneGenerator name
        zoneType     | Type of zone            | yes       |
        select       | Select either the inside or outside | no | inside
        moveUpdate   | Switch to update after mesh motion  | no | false
        centre       | Centre of the sphere    | yes       |
        radius       | Radius of the sphere    | yes       |
        zone         | Optional zone to sub-set            | no  |
        zones        | Optional list of zones to sub-set   | no  |
    \endtable

    A cellZone named \c sphere1 containing the cells with centres inside
    a cylinder can be generated by
    \verbatim
        sphere1
        {
            type        sphere;
            zoneType    cell;

            centre      (-0.003 0.0025 0);
            radius      0.001;
        }
    \endverbatim

See also
    zoneGenerators::volume

SourceFiles
    sphere.C

\*---------------------------------------------------------------------------*/

#ifndef sphere_zoneGenerator_H
#define sphere_zoneGenerator_H

#include "volume.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace zoneGenerators
{

/*---------------------------------------------------------------------------*\
                              Class sphere Declaration
\*---------------------------------------------------------------------------*/

class sphere
:
    public volume
{
    // Private Data

        //- Centre
        vector centre_;

        //- Radius
        scalar radius_;

        //- Radius squared
        scalar radiusSqr_;


    // Private Member Functions

        friend class volume;

        //- Return true if the spherees contain the given point
        inline bool contains(const point& p) const;


public:

    //- Runtime type information
    TypeName("sphere");


    // Constructors

        //- Construct from dictionary
        sphere
        (
            const word& name,
            const polyMesh& mesh,
            const dictionary& dict
        );


    //- Destructor
    virtual ~sphere();


    // Member Functions

        virtual zoneSet generate() const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace zoneGenerators
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
