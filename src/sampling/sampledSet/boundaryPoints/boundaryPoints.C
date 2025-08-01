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

\*---------------------------------------------------------------------------*/

#include "boundaryPoints.H"
#include "polyMesh.H"
#include "addToRunTimeSelectionTable.H"
#include "treeBoundBox.H"
#include "treeDataFace.H"
#include "Time.H"
#include "OBJstream.H"
#include "RemoteData.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace sampledSets
{
    defineTypeNameAndDebug(boundaryPoints, 0);
    addToRunTimeSelectionTable(sampledSet, boundaryPoints, word);
}
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

bool Foam::sampledSets::boundaryPoints::calcSamples
(
    DynamicList<point>& samplingPositions,
    DynamicList<scalar>&,
    DynamicList<label>& samplingSegments,
    DynamicList<label>& samplingCells,
    DynamicList<label>& samplingFaces
) const
{
    // Get the patch IDs
    const labelHashSet patchIDs(mesh().boundaryMesh().patchSet(patches_));

    // Construct a single list of all patch faces
    label nPatchFaces = 0;
    forAllConstIter(labelHashSet, patchIDs, iter)
    {
        const polyPatch& pp = mesh().boundaryMesh()[iter.key()];
        nPatchFaces += pp.size();
    }
    labelList patchFaces(nPatchFaces);
    nPatchFaces = 0;
    forAllConstIter(labelHashSet, patchIDs, iter)
    {
        const polyPatch& pp = mesh().boundaryMesh()[iter.key()];
        forAll(pp, i)
        {
            patchFaces[nPatchFaces++] = pp.start()+i;
        }
    }

    // Construct a processor-local bound box
    treeBoundBox patchBB(point::max, point::min);
    forAllConstIter(labelHashSet, patchIDs, iter)
    {
        const polyPatch& pp = mesh().boundaryMesh()[iter.key()];
        const boundBox patchBb(pp.points(), pp.meshPoints(), false);
        patchBB.min() = min(patchBB.min(), patchBb.min());
        patchBB.max() = max(patchBB.max(), patchBb.max());
    }
    patchBB = patchBB.extend(1e-4);

    // Create a search tree
    indexedOctree<treeDataFace> patchTree
    (
        treeDataFace    // all information needed to search faces
        (
            false,      // do not cache the bound box
            mesh(),
            patchFaces
        ),
        patchBB,        // overall search box
        8,              // maximum number of levels
        10,             // how many elements per leaf
        3.0             // in how many leaves is a shape on average
    );

    // Force calculation of face-diagonal decomposition
    (void)mesh().tetBasePtIs();

    // Generate the nearest patch information for each sampling point
    List<RemoteData<Tuple2<scalar, point>>> nearest(points_.size());
    forAll(points_, sampleI)
    {
        const point& sample = points_[sampleI];

        const pointIndexHit pih =
            patchFaces.size()
          ? patchTree.findNearest(sample, sqr(maxDistance_))
          : pointIndexHit();

        if (pih.hit())
        {
            nearest[sampleI].proci = Pstream::myProcNo();
            nearest[sampleI].elementi = patchFaces[pih.index()];
            nearest[sampleI].data.first() = magSqr(pih.hitPoint() - sample);
            nearest[sampleI].data.second() = pih.hitPoint();
        }
    }

    // Reduce to get the nearest patch locations globally
    Pstream::listCombineGather
    (
        nearest,
        RemoteData<Tuple2<scalar, point>>::smallestFirstEqOp()
    );
    Pstream::listCombineScatter(nearest);

    // Dump connecting lines from the sampling points to the hit locations
    if (debug && Pstream::master())
    {
        OBJstream str(mesh().time().path()/(name() + "_nearest.obj"));

        forAll(nearest, sampleI)
        {
            if (nearest[sampleI].proci != -1)
            {
                str.write
                (
                    linePointRef
                    (
                        points_[sampleI],
                        nearest[sampleI].data.second()
                    )
                );
            }
        }
    }

    // Store the sampling locations on the nearest processor
    forAll(nearest, sampleI)
    {
        if (nearest[sampleI].proci != -1)
        {
            if (nearest[sampleI].proci == Pstream::myProcNo())
            {
                const label facei = nearest[sampleI].elementi;

                samplingPositions.append(nearest[sampleI].data.second());
                samplingSegments.append(sampleI);
                samplingCells.append(mesh().faceOwner()[facei]);
                samplingFaces.append(facei);
            }
        }
        else
        {
            WarningInFunction
                << "Unable to find location on patches " << patches_
                << " for the point " << points_[sampleI]
                << " within a distance of " << maxDistance_ << endl;
        }
    }

    // This set is unordered. Distances have not been created.
    return false;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::sampledSets::boundaryPoints::boundaryPoints
(
    const word& name,
    const polyMesh& mesh,
    const dictionary& dict
)
:
    sampledSet(name, mesh, dict),
    points_(dict.lookup("points")),
    patches_(dict.lookup("patches")),
    maxDistance_(dict.lookup<scalar>("maxDistance"))
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::sampledSets::boundaryPoints::~boundaryPoints()
{}


// ************************************************************************* //
