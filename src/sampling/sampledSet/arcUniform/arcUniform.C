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

#include "arcUniform.H"
#include "sampledSet.H"
#include "DynamicList.H"
#include "polyMesh.H"
#include "addToRunTimeSelectionTable.H"
#include "word.H"
#include "points.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace sampledSets
{
    defineTypeNameAndDebug(arcUniform, 0);
    addToRunTimeSelectionTable(sampledSet, arcUniform, word);
}
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

bool Foam::sampledSets::arcUniform::calcSamples
(
    DynamicList<point>& samplingPositions,
    DynamicList<scalar>& samplingDistances,
    DynamicList<label>& samplingSegments,
    DynamicList<label>& samplingCells,
    DynamicList<label>& samplingFaces
) const
{
    // Get the coordinate system
    const vector axis1 = radial_ - (radial_ & normal_)*normal_;
    const vector axis2 = normal_ ^ axis1;
    const scalar radius = mag(axis1);

    // Compute all point locations
    const scalarField ts(scalarList(identityMap(nPoints_))/(nPoints_ - 1));
    const scalarField theta((1 - ts)*startAngle_ + ts*endAngle_);
    const scalarField c(cos(theta)), s(sin(theta));
    const pointField points(centre_ + c*axis1 + s*axis2);

    // Calculate the sampling topology
    points::calcSamples
    (
        mesh(),
        points,
        samplingPositions,
        samplingDistances,
        samplingSegments,
        samplingCells,
        samplingFaces
    );

    // Overwrite the distances
    forAll(samplingPositions, i)
    {
        const vector v = samplingPositions[i] - centre_;
        const scalar theta = atan2(v & axis2, v & axis1);
        samplingDistances[i] = radius*theta;
    }

    // This set is ordered. Distances have been created.
    return true;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::sampledSets::arcUniform::arcUniform
(
    const word& name,
    const polyMesh& mesh,
    const dictionary& dict
)
:
    sampledSet(name, mesh, dict),
    centre_(dict.lookup("centre")),
    normal_(normalised(dict.lookup<vector>("normal"))),
    radial_(dict.lookup<vector>("radial")),
    startAngle_(dict.lookup<scalar>("startAngle")),
    endAngle_(dict.lookup<scalar>("endAngle")),
    nPoints_(dict.lookup<scalar>("nPoints"))
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::sampledSets::arcUniform::~arcUniform()
{}


// ************************************************************************* //
