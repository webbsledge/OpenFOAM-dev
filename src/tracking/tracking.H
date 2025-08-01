/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2024-2025 OpenFOAM Foundation
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

Namespace
    Foam::tracking

Description
    Functions for tracking locations through a mesh

SourceFiles
    tracking.C

\*---------------------------------------------------------------------------*/

#ifndef tracking_H
#define tracking_H

#include "polyMesh.H"
#include "barycentric.H"
#include "barycentricTensor.H"
#include "vector.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class meshSearch;
class wedgePolyPatch;
class cyclicPolyPatch;
class processorPolyPatch;

namespace tracking
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Geometry

    //- Return the position given the coordinates and tet topology
    inline point position
    (
        const polyMesh& mesh,
        const barycentric& coordinates,
        const label celli,
        const label facei,
        const label faceTrii,
        const scalar stepFraction
    );

    //- Return the coordinates given the position and tet topology
    barycentric coordinates
    (
        const polyMesh& mesh,
        const point& position,
        const label celli,
        const label facei,
        const label faceTrii,
        const scalar stepFraction
    );

    //- Return the normal of the corresponding point on the associated face and
    //  the displacement of that point over the time-step
    Pair<vector> faceNormalAndDisplacement
    (
        const polyMesh& mesh,
        const barycentric& coordinates,
        const label celli,
        const label facei,
        const label faceTrii,
        const scalar stepFraction
    );


// Tracking

    //- Track along the displacement for a given fraction of the overall
    //  time-step. End when the track is complete or when a face is hit. Return
    //  whether or not a face was hit (true) or the track completed (false) and
    //  the proportion of the displacement still to be completed. Displacement
    //  can be a single vector, d, in which case the path is linear and equal
    //  to x0 + lambda*d, where x0 is the starting position and lambda is the
    //  local coordinate along the path. Or, displacement can be a pair of
    //  vectors, d1 and d2, in which case the path is parabolic and equal to
    //  x0 + lambda*d1 + lambda*lambda*d2.
    template<class Displacement>
    Tuple2<bool, scalar> toFace
    (
        const polyMesh& mesh,
        const Displacement& displacement,
        const scalar fraction,
        barycentric& coordinates,
        label& celli,
        label& facei,
        label& faceTrii,
        scalar& stepFraction,
        scalar& stepFractionBehind,
        label& nTracksBehind,
        const string& debugPrefix = NullObjectRef<string>()
    );

    //- As toFace, except that if the track ends on an internal face then this
    //  face will be crossed
    template<class Displacement>
    Tuple2<bool, scalar> toCell
    (
        const polyMesh& mesh,
        const Displacement& displacement,
        const scalar fraction,
        barycentric& coordinates,
        label& celli,
        label& facei,
        label& faceTrii,
        scalar& stepFraction,
        scalar& stepFractionBehind,
        label& nTracksBehind,
        const string& debugPrefix = NullObjectRef<string>()
    );

    //- As toFace, except that the track continues across multiple cells until
    //  it ends or until a boundary face is hit
    template<class Displacement>
    Tuple2<bool, scalar> toBoundary
    (
        const polyMesh& mesh,
        const Displacement& displacement,
        const scalar fraction,
        barycentric& coordinates,
        label& celli,
        label& facei,
        label& faceTrii,
        scalar& stepFraction,
        scalar& stepFractionBehind,
        label& nTracksBehind,
        const string& debugPrefix = NullObjectRef<string>()
    );


// Initialisation

    //- Initialise the location at the given position. Returns whether or not a
    //  boundary was hit. The cell index must be valid.
    bool locate
    (
        const polyMesh& mesh,
        const point& position,
        barycentric& coordinates,
        label& celli,
        label& facei,
        label& faceTrii,
        const scalar stepFraction,
        const string& debugPrefix = NullObjectRef<string>()
    );

    //- Initialise the location at the given position. Returns whether or not a
    //  boundary was hit. The cell index may be invalid, in which case a tree
    //  search is performed to find the cell containing the position.
    bool locate
    (
        const meshSearch& searchEngine,
        const point& position,
        barycentric& coordinates,
        label& celli,
        label& facei,
        label& faceTrii,
        const scalar stepFraction,
        const string& debugPrefix = NullObjectRef<string>()
    );


// Topology Changes

    //- Cross an internal face
    void crossInternalFace
    (
        const polyMesh& mesh,
        barycentric& coordinates,
        label& celli,
        label& facei,
        label& faceTrii
    );

    //- Cross a wedge patch
    void crossWedge
    (
        const wedgePolyPatch& inPatch,
        barycentric& coordinates,
        label& celli,
        label& facei,
        label& faceTrii,
        const scalar stepFraction
    );

    //- Cross a cyclic patch
    void crossCyclic
    (
        const cyclicPolyPatch& inPatch,
        barycentric& coordinates,
        label& celli,
        label& facei,
        label& faceTrii
    );

    //- Initialise crossing of a processor patch. Breaks the topology in order
    //  to store the destination patch face in advance of communication.
    void inProcessor
    (
        const processorPolyPatch& inPatch,
        label& celli,
        label& facei
    );

    //- Complete crossing of a processor patch. Restore the topology.
    void outProcessor
    (
        const processorPolyPatch& outPatch,
        barycentric& coordinates,
        label& celli,
        label& facei,
        label& faceTrii
    );

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace tracking
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "trackingI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
