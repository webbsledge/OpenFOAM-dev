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
    Foam::blockMesh

Description
    A multi-block mesh generator

    Note:
        The vertices, cells and patches for filling the blocks are
        demand-driven.

SourceFiles
    blockMesh.C
    blockMeshCheck.C
    blockMeshCreate.C
    blockMeshMerge.C
    blockMeshTopology.C

\*---------------------------------------------------------------------------*/

#ifndef blockMesh_H
#define blockMesh_H

#include "blockList.H"
#include "searchableSurfaceList.H"
#include "polyMesh.H"
#include "IOdictionary.H"
#include "blockVertexList.H"
#include "blockEdgeList.H"
#include "blockFaceList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                          Class blockMesh Declaration
\*---------------------------------------------------------------------------*/

class blockMesh
:
    public blockList
{
    // Private Data

        //- Reference to mesh dictionary
        const IOdictionary& meshDict_;

        //- Switch for verbose output
        bool verboseOutput;

        //- Switch checking face consistency (defaults to true)
        bool checkFaceCorrespondence_;

        //- Optional searchable geometry to project face-points to
        searchableSurfaceList geometry_;

        //- The scaling factor to convert to meters
        scalar scaleFactor_;

        //- The list of block vertices
        blockVertexList blockVertices_;

        //- The list of block vertex positions
        pointField vertices_;

        //- The list of curved edges
        blockEdgeList edges_;

        //- The list of curved faces
        blockFaceList faces_;

        //- The blocks themselves (the topology) as a polyMesh
        polyMesh* topologyPtr_;

        label nPoints_;

        //- The sum of all cells in each block
        label nCells_;

        //- The point offset added to each block
        labelList blockOffsets_;

        //- The merge points information
        labelList mergeList_;

        mutable pointField points_;

        mutable cellShapeList cells_;

        mutable faceListList patches_;


    // Private Member Functions

        template<class Source>
        void checkPatchLabels
        (
            const Source& source,
            const word& patchName,
            const pointField& points,
            faceList& patchShapes
        ) const;

        void readPatches
        (
            const dictionary& meshDescription,
            faceListList& tmpBlocksPatches,
            wordList& patchNames,
            wordList& patchTypes,
            wordList& nbrPatchNames
        );

        void readBoundary
        (
            const dictionary& meshDescription,
            wordList& patchNames,
            faceListList& tmpBlocksPatches,
            PtrList<dictionary>& patchDicts
        );

        void createCellShapes(cellShapeList& tmpBlockCells);

        void defaultPatchError
        (
            const word& defaultPatchName,
            const dictionary& meshDescription
        ) const;

        polyMesh* createTopology
        (
            const IOdictionary&,
            const fileName& meshPath,
            const word& regionName
        );

        void check(const polyMesh&, const dictionary&) const;

        //- Determine the merge info and the final number of cells/points
        void calcMergeInfo();

        //- Determine the merge info and the final number of cells/points
        void calcMergeInfoFast();

        faceList createPatchFaces(const polyPatch& patchTopologyFaces) const;

        Pair<scalar> xCellSizes
        (
            const block& b,
            const pointField& blockPoints,
            const label j,
            const label k
        ) const;

        Pair<scalar> yCellSizes
        (
            const block& b,
            const pointField& blockPoints,
            const label i,
            const label k
        ) const;

        Pair<scalar> zCellSizes
        (
            const block& b,
            const pointField& blockPoints,
            const label i,
            const label j
        ) const;

        void printCellSizeRange(const Pair<scalar>& cellSizes) const;

        void printCellSizeRanges
        (
            const direction d,
            const FixedList<Pair<scalar>, 4>& cellSizes
        ) const;

        void createPoints() const;
        void createCells() const;
        void createPatches() const;


public:

    // Static Data Members

        ClassName("blockMesh");

        //- Switch checking block face orientation
        //  to ensure that all faces are outward-pointing.
        //  This check may fail if the block is intentionally very twisted
        //  for curved edges to be applied and can be switched off.
        static Switch checkBlockFaceOrientation;


    // Constructors

        //- Construct from IOdictionary, path and region name
        blockMesh
        (
            const IOdictionary&,
            const fileName& meshPath,
            const word& regionName
        );

        //- Disallow default bitwise copy construction
        blockMesh(const blockMesh&) = delete;


    //- Destructor
    ~blockMesh();


    // Member Functions

        // Access

            //- Access to input dictionary
            const dictionary& meshDict() const
            {
                return meshDict_;
            }

            //- Optional searchable geometry to project face-points to
            const searchableSurfaceList& geometry() const
            {
                return geometry_;
            }

            //- Reference to point field defining the blockMesh
            //  these points have not been scaled by scaleFactor
            const pointField& vertices() const;

            //- Return the blockMesh topology as a polyMesh
            const polyMesh& topology() const;

            //- Return the curved edges
            const blockEdgeList& edges() const
            {
                return edges_;
            }

            //- Return the curved faces
            const blockFaceList& faces() const
            {
                return faces_;
            }

            //- The scaling factor used to convert to meters
            scalar scaleFactor() const;

            //- The points for the entire mesh
            //  these points have been scaled by scaleFactor
            const pointField& points() const;

            //- Return cell shapes list
            const cellShapeList& cells() const;

            //- Return the patch face lists
            const faceListList& patches() const;

            //- Get patch information from the topology mesh
            PtrList<dictionary> patchDicts() const;

            //- Return patch names
            wordList patchNames() const;

            //- Number of blocks with specified zones
            label numZonedBlocks() const;


        // Edit

            //- Enable/disable verbose information about the progress
            void verbose(const bool on=true);


        // Write

            //- Writes edges of blockMesh in OBJ format.
            void writeTopology(Ostream&) const;


    // Member Operators

        //- Disallow default bitwise assignment
        void operator=(const blockMesh&) = delete;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
