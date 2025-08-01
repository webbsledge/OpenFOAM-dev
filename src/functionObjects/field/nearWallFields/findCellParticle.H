/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2013-2025 OpenFOAM Foundation
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
    Foam::findCellParticle

Description
    Particle class that finds cells by tracking

SourceFiles
    findCellParticle.C

\*---------------------------------------------------------------------------*/

#ifndef findCellParticle_H
#define findCellParticle_H

#include "particle.H"
#include "Cloud.H"
#include "autoPtr.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class findCellParticleCloud;


// Forward declaration of friend functions and operators

class findCellParticle;

Ostream& operator<<(Ostream&, const findCellParticle&);


/*---------------------------------------------------------------------------*\
                      Class findCellParticle Declaration
\*---------------------------------------------------------------------------*/

class findCellParticle
:
    public particle
{
    // Private Data

        //- Displacement over which to track
        vector displacement_;

        //- Passive data
        label data_;


public:

    friend class lagrangian::Cloud<findCellParticle>;

    //- Class used to pass tracking data to the trackToFace function
    class trackingData
    :
        public particle::trackingData
    {
        labelListList& cellToData_;
        List<List<point>>& cellToEnd_;

    public:

        // Constructors

            trackingData
            (
                lagrangian::Cloud<findCellParticle>& cloud,
                labelListList& cellToData,
                List<List<point>>& cellToEnd
            )
            :
                particle::trackingData(cloud),
                cellToData_(cellToData),
                cellToEnd_(cellToEnd)
            {}


        // Member Functions

            labelListList& cellToData()
            {
                return cellToData_;
            }

            List<List<point>>& cellToEnd()
            {
                return cellToEnd_;
            }
    };


    // Constructors

        //- Construct from a position and a cell, searching for the rest of the
        //  required topology
        findCellParticle
        (
            const meshSearch& searchEngine,
            const vector& position,
            const label celli,
            label& nLocateBoundaryHits,
            const vector& displacement,
            const label data
        );

        //- Construct from Istream
        findCellParticle(Istream& is, bool readFields = true);

        //- Construct and return a clone
        autoPtr<particle> clone() const
        {
            return autoPtr<particle>(new findCellParticle(*this));
        }

        //- Construct from Istream and return
        static autoPtr<findCellParticle> New(Istream& is)
        {
            return autoPtr<findCellParticle>(new findCellParticle(is));
        }


    // Member Functions

        //- Displacement over which to track
        const vector& displacement() const
        {
            return displacement_;
        }

        //- Displacement over which to track
        vector& displacement()
        {
            return displacement_;
        }

        //- Transported label
        label data() const
        {
            return data_;
        }

        //- Transported label
        label& data()
        {
            return data_;
        }


        // Tracking

            //- Track all particles to their end point
            bool move(lagrangian::Cloud<findCellParticle>&, trackingData&);

            //- Overridable function to handle the particle hitting a wedge
            void hitWedgePatch
            (
                lagrangian::Cloud<findCellParticle>&,
                trackingData&
            );

            //- Overridable function to handle the particle hitting a
            //  symmetry plane
            void hitSymmetryPlanePatch
            (
                lagrangian::Cloud<findCellParticle>&,
                trackingData&
            );

            //- Overridable function to handle the particle hitting a
            //  symmetry patch
            void hitSymmetryPatch
            (
                lagrangian::Cloud<findCellParticle>&,
                trackingData&
            );

            //- Overridable function to handle the particle hitting a cyclic
            void hitCyclicPatch
            (
                lagrangian::Cloud<findCellParticle>&,
                trackingData&
            );

            //- Overridable function to handle the particle hitting a
            //- processorPatch
            void hitProcessorPatch
            (
                lagrangian::Cloud<findCellParticle>&,
                trackingData&
            );

            //- Overridable function to handle the particle hitting a wallPatch
            void hitWallPatch
            (
                lagrangian::Cloud<findCellParticle>&,
                trackingData&
            );


    // Ostream Operator

        friend Ostream& operator<<(Ostream&, const findCellParticle&);
};


template<>
inline bool contiguous<findCellParticle>()
{
    return true;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
