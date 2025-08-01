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

#include "polyMesh.H"
#include "OSspecific.H"
#include "Time.H"
#include "cellIOList.H"
#include "wedgePolyPatch.H"
#include "emptyPolyPatch.H"
#include "globalMeshData.H"
#include "processorPolyPatch.H"
#include "polyMeshTetDecomposition.H"
#include "meshObjects.H"
#include "pointMesh.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(polyMesh, 0);
}


Foam::word Foam::polyMesh::defaultRegion = "region0";


Foam::word Foam::polyMesh::meshSubDir = "polyMesh";


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

Foam::fileName Foam::polyMesh::regionDir(const IOobject& io)
{
    if (io.name() == defaultRegion)
    {
        return io.db().dbDir()/io.local();
    }
    else
    {
        return io.db().dbDir()/io.local()/io.name();
    }
}


void Foam::polyMesh::calcDirections() const
{
    for (direction cmpt=0; cmpt<vector::nComponents; cmpt++)
    {
        solutionD_[cmpt] = 1;
    }

    // Knock out empty and wedge directions. Note:they will be present on all
    // domains.

    label nEmptyPatches = 0;
    label nWedgePatches = 0;

    vector emptyDirVec = Zero;
    vector wedgeDirVec = Zero;

    forAll(boundaryMesh(), patchi)
    {
        if (boundaryMesh()[patchi].size())
        {
            if (isA<emptyPolyPatch>(boundaryMesh()[patchi]))
            {
                nEmptyPatches++;
                emptyDirVec += sum(cmptMag(boundaryMesh()[patchi].faceAreas()));
            }
            else if (isA<wedgePolyPatch>(boundaryMesh()[patchi]))
            {
                const wedgePolyPatch& wpp = refCast<const wedgePolyPatch>
                (
                    boundaryMesh()[patchi]
                );

                nWedgePatches++;
                wedgeDirVec += cmptMag(wpp.centreNormal());
            }
        }
    }

    reduce(nEmptyPatches, maxOp<label>());
    reduce(nWedgePatches, maxOp<label>());

    if (nEmptyPatches)
    {
        reduce(emptyDirVec, sumOp<vector>());

        emptyDirVec /= mag(emptyDirVec);

        for (direction cmpt=0; cmpt<vector::nComponents; cmpt++)
        {
            if (emptyDirVec[cmpt] > 1e-6)
            {
                solutionD_[cmpt] = -1;
            }
            else
            {
                solutionD_[cmpt] = 1;
            }
        }
    }


    // Knock out wedge directions

    geometricD_ = solutionD_;

    if (nWedgePatches)
    {
        reduce(wedgeDirVec, sumOp<vector>());

        wedgeDirVec /= mag(wedgeDirVec);

        for (direction cmpt=0; cmpt<vector::nComponents; cmpt++)
        {
            if (wedgeDirVec[cmpt] > 1e-6)
            {
                geometricD_[cmpt] = -1;
            }
            else
            {
                geometricD_[cmpt] = 1;
            }
        }
    }
}


Foam::autoPtr<Foam::labelIOList> Foam::polyMesh::readTetBasePtIs() const
{
    typeIOobject<labelIOList> io
    (
        "tetBasePtIs",
        instance(),
        meshSubDir,
        *this,
        IOobject::READ_IF_PRESENT,
        IOobject::NO_WRITE
    );

    if (io.headerOk())
    {
        return autoPtr<labelIOList>(new labelIOList(io));
    }
    else
    {
        return autoPtr<labelIOList>(nullptr);
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::polyMesh::polyMesh(const IOobject& io)
:
    objectRegistry(io, regionDir(io)),
    primitiveMesh(),
    points_
    (
        IOobject
        (
            "points",
            time().findInstance(meshDir(), "points"),
            meshSubDir,
            *this,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        )
    ),
    faces_
    (
        IOobject
        (
            "faces",
            time().findInstance(meshDir(), "faces"),
            meshSubDir,
            *this,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        )
    ),
    owner_
    (
        IOobject
        (
            "owner",
            faces_.instance(),
            meshSubDir,
            *this,
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        )
    ),
    neighbour_
    (
        IOobject
        (
            "neighbour",
            faces_.instance(),
            meshSubDir,
            *this,
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        )
    ),
    clearedPrimitives_(false),
    boundary_
    (
        IOobject
        (
            "boundary",
            faces_.instance(),
            meshSubDir,
            *this,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        ),
        *this
    ),
    bounds_(points_),
    comm_(UPstream::worldComm),
    geometricD_(Zero),
    solutionD_(Zero),
    tetBasePtIsPtr_(readTetBasePtIs()),
    pointZones_
    (
        IOobject
        (
            "pointZones",
            time().findInstance
            (
                meshDir(),
                "pointZones",
                IOobject::READ_IF_PRESENT,
                faces_.instance()
            ),
            meshSubDir,
            *this,
            IOobject::NO_READ, // Delay reading
            IOobject::NO_WRITE
        ),
        *this
    ),
    faceZones_
    (
        IOobject
        (
            "faceZones",
            time().findInstance
            (
                meshDir(),
                "faceZones",
                IOobject::READ_IF_PRESENT,
                faces_.instance()
            ),
            meshSubDir,
            *this,
            IOobject::NO_READ, // Delay reading
            IOobject::NO_WRITE
        ),
        *this
    ),
    cellZones_
    (
        IOobject
        (
            "cellZones",
            time().findInstance
            (
                meshDir(),
                "cellZones",
                IOobject::READ_IF_PRESENT,
                faces_.instance()
            ),
            meshSubDir,
            *this,
            IOobject::NO_READ, // Delay reading
            IOobject::NO_WRITE
        ),
        *this
    ),
    globalMeshDataPtr_(nullptr),
    curMotionTimeIndex_(-1),
    oldPointsPtr_(nullptr),
    oldCellCentresPtr_(nullptr),
    storeOldCellCentres_(false),
    moving_(false),
    topoChanged_(false)
{
    if (!owner_.headerClassName().empty())
    {
        initMesh();
    }
    else
    {
        cellCompactIOList cLst
        (
            IOobject
            (
                "cells",
                faces_.instance(),
                meshSubDir,
                *this,
                IOobject::MUST_READ,
                IOobject::NO_WRITE
            )
        );

        // Set the primitive mesh
        initMesh(cLst);

        owner_.write();
        neighbour_.write();
    }

    // Calculate topology for the patches (processor-processor comms etc.)
    boundary_.topoChange();

    // Calculate the geometry for the patches (transformation tensors etc.)
    boundary_.calcGeometry();

    // Warn if global empty mesh
    const bool complete = Pstream::parRun() || !time().processorCase();
    if (complete && returnReduce(nPoints(), sumOp<label>()) == 0)
    {
        WarningInFunction
            << "no points in mesh" << endl;
    }
    if (complete && returnReduce(nCells(), sumOp<label>()) == 0)
    {
        WarningInFunction
            << "no cells in mesh" << endl;
    }

    // Initialise demand-driven data
    calcDirections();

    // Read the zones now that the mesh geometry is available to construct them
    pointZones_.readIfPresent();
    faceZones_.readIfPresent();
    cellZones_.readIfPresent();
}


Foam::polyMesh::polyMesh
(
    const IOobject& io,
    pointField&& points,
    faceList&& faces,
    labelList&& owner,
    labelList&& neighbour,
    const bool syncPar
)
:
    objectRegistry(io, regionDir(io)),
    primitiveMesh(),
    points_
    (
        IOobject
        (
            "points",
            instance(),
            meshSubDir,
            *this,
            io.readOpt(),
            IOobject::AUTO_WRITE
        ),
        move(points)
    ),
    faces_
    (
        IOobject
        (
            "faces",
            instance(),
            meshSubDir,
            *this,
            io.readOpt(),
            IOobject::AUTO_WRITE
        ),
        move(faces)
    ),
    owner_
    (
        IOobject
        (
            "owner",
            instance(),
            meshSubDir,
            *this,
            io.readOpt(),
            IOobject::AUTO_WRITE
        ),
        move(owner)
    ),
    neighbour_
    (
        IOobject
        (
            "neighbour",
            instance(),
            meshSubDir,
            *this,
            io.readOpt(),
            IOobject::AUTO_WRITE
        ),
        move(neighbour)
    ),
    clearedPrimitives_(false),
    boundary_
    (
        IOobject
        (
            "boundary",
            instance(),
            meshSubDir,
            *this,
            io.readOpt(),
            IOobject::AUTO_WRITE
        ),
        *this,
        polyPatchList()
    ),
    bounds_(points_, syncPar),
    comm_(UPstream::worldComm),
    geometricD_(Zero),
    solutionD_(Zero),
    tetBasePtIsPtr_(readTetBasePtIs()),
    pointZones_
    (
        IOobject
        (
            "pointZones",
            instance(),
            meshSubDir,
            *this,
            io.readOpt(),
            IOobject::NO_WRITE
        ),
        *this
    ),
    faceZones_
    (
        IOobject
        (
            "faceZones",
            instance(),
            meshSubDir,
            *this,
            io.readOpt(),
            IOobject::NO_WRITE
        ),
        *this
    ),
    cellZones_
    (
        IOobject
        (
            "cellZones",
            instance(),
            meshSubDir,
            *this,
            io.readOpt(),
            IOobject::NO_WRITE
        ),
        *this
    ),
    globalMeshDataPtr_(nullptr),
    curMotionTimeIndex_(-1),
    oldPointsPtr_(nullptr),
    oldCellCentresPtr_(nullptr),
    storeOldCellCentres_(false),
    moving_(false),
    topoChanged_(false)
{
    // Check if the faces and cells are valid
    forAll(faces_, facei)
    {
        const face& curFace = faces_[facei];

        if (min(curFace) < 0 || max(curFace) > points_.size())
        {
            FatalErrorInFunction
                << "Face " << facei << "contains vertex labels out of range: "
                << curFace << " Max point index = " << points_.size()
                << abort(FatalError);
        }
    }

    // Set the primitive mesh
    initMesh();
}


Foam::polyMesh::polyMesh
(
    const IOobject& io,
    pointField&& points,
    faceList&& faces,
    cellList&& cells,
    const bool syncPar
)
:
    objectRegistry(io, regionDir(io)),
    primitiveMesh(),
    points_
    (
        IOobject
        (
            "points",
            instance(),
            meshSubDir,
            *this,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        move(points)
    ),
    faces_
    (
        IOobject
        (
            "faces",
            instance(),
            meshSubDir,
            *this,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        move(faces)
    ),
    owner_
    (
        IOobject
        (
            "owner",
            instance(),
            meshSubDir,
            *this,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        label(0)
    ),
    neighbour_
    (
        IOobject
        (
            "neighbour",
            instance(),
            meshSubDir,
            *this,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        label(0)
    ),
    clearedPrimitives_(false),
    boundary_
    (
        IOobject
        (
            "boundary",
            instance(),
            meshSubDir,
            *this,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        *this,
        0
    ),
    bounds_(points_, syncPar),
    comm_(UPstream::worldComm),
    geometricD_(Zero),
    solutionD_(Zero),
    tetBasePtIsPtr_(readTetBasePtIs()),
    pointZones_
    (
        IOobject
        (
            "pointZones",
            instance(),
            meshSubDir,
            *this,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        *this
    ),
    faceZones_
    (
        IOobject
        (
            "faceZones",
            instance(),
            meshSubDir,
            *this,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        *this
    ),
    cellZones_
    (
        IOobject
        (
            "cellZones",
            instance(),
            meshSubDir,
            *this,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        *this
    ),
    globalMeshDataPtr_(nullptr),
    curMotionTimeIndex_(-1),
    oldPointsPtr_(nullptr),
    oldCellCentresPtr_(nullptr),
    storeOldCellCentres_(false),
    moving_(false),
    topoChanged_(false)
{
    // Check if faces are valid
    forAll(faces_, facei)
    {
        const face& curFace = faces_[facei];

        if (min(curFace) < 0 || max(curFace) > points_.size())
        {
            FatalErrorInFunction
                << "Face " << facei << "contains vertex labels out of range: "
                << curFace << " Max point index = " << points_.size()
                << abort(FatalError);
        }
    }

    // transfer in cell list
    cellList cLst(move(cells));

    // Check if cells are valid
    forAll(cLst, celli)
    {
        const cell& curCell = cLst[celli];

        if (min(curCell) < 0 || max(curCell) > faces_.size())
        {
            FatalErrorInFunction
                << "Cell " << celli << "contains face labels out of range: "
                << curCell << " Max face index = " << faces_.size()
                << abort(FatalError);
        }
    }

    // Set the primitive mesh
    initMesh(cLst);
}


Foam::polyMesh::polyMesh(polyMesh&& mesh)
:
    objectRegistry(move(mesh)),
    primitiveMesh(move(mesh)),
    points_(move(mesh.points_)),
    faces_(move(mesh.faces_)),
    owner_(move(mesh.owner_)),
    neighbour_(move(mesh.neighbour_)),
    clearedPrimitives_(mesh.clearedPrimitives_),
    boundary_(move(mesh.boundary_)),
    bounds_(move(mesh.bounds_)),
    comm_(mesh.comm_),
    geometricD_(mesh.geometricD_),
    solutionD_(mesh.solutionD_),
    tetBasePtIsPtr_(move(mesh.tetBasePtIsPtr_)),
    pointZones_(move(mesh.pointZones_)),
    faceZones_(move(mesh.faceZones_)),
    cellZones_(move(mesh.cellZones_)),
    globalMeshDataPtr_(move(mesh.globalMeshDataPtr_)),
    curMotionTimeIndex_(mesh.curMotionTimeIndex_),
    oldPointsPtr_(move(mesh.oldPointsPtr_)),
    oldCellCentresPtr_(move(mesh.oldCellCentresPtr_)),
    storeOldCellCentres_(mesh.storeOldCellCentres_),
    moving_(mesh.moving_),
    topoChanged_(mesh.topoChanged_)
{}


void Foam::polyMesh::resetPrimitives
(
    pointField&& points,
    faceList&& faces,
    labelList&& owner,
    labelList&& neighbour,
    const labelList& patchSizes,
    const labelList& patchStarts,
    const bool validBoundary
)
{
    // Clear addressing. Keep geometric props and updateable props for mapping.
    clearAddressing(true);

    // Take over new primitive data.
    // Optimised to avoid overwriting data at all
    if (notNull(points))
    {
        points_ = move(points);
        bounds_ = boundBox(points_, validBoundary);
    }

    if (notNull(faces))
    {
        faces_ = move(faces);
    }

    if (notNull(owner))
    {
        owner_ = move(owner);
    }

    if (notNull(neighbour))
    {
        neighbour_ = move(neighbour);
    }


    // Reset patch sizes and starts
    forAll(boundary_, patchi)
    {
        boundary_[patchi] = polyPatch
        (
            boundary_[patchi],
            boundary_,
            patchi,
            patchSizes[patchi],
            patchStarts[patchi]
        );
    }


    // Flags the mesh files as being changed
    setInstance(time().name());

    // Check if the faces and cells are valid
    forAll(faces_, facei)
    {
        const face& curFace = faces_[facei];

        if (min(curFace) < 0 || max(curFace) > points_.size())
        {
            FatalErrorInFunction
                << "Face " << facei << " contains vertex labels out of range: "
                << curFace << " Max point index = " << points_.size()
                << abort(FatalError);
        }
    }


    // Set the primitive mesh from the owner_, neighbour_.
    // Works out from patch end where the active faces stop.
    initMesh();


    if (validBoundary)
    {
        // Note that we assume that all the patches stay the same and are
        // correct etc. so we can already use the patches to do
        // processor-processor comms.

        // Calculate topology for the patches (processor-processor comms etc.)
        boundary_.topoChange();

        // Calculate the geometry for the patches (transformation tensors etc.)
        boundary_.calcGeometry();

        // Warn if global empty mesh
        if
        (
            (returnReduce(nPoints(), sumOp<label>()) == 0)
         || (returnReduce(nCells(), sumOp<label>()) == 0)
        )
        {
            FatalErrorInFunction
                << "no points or no cells in mesh"
                << exit(FatalError);
        }
    }
}


void Foam::polyMesh::swap(polyMesh& otherMesh)
{
    // Clear addressing. Keep geometric and updatable properties for mapping.
    clearAddressing(true);
    otherMesh.clearAddressing(true);

    // Swap the primitives
    points_.swap(otherMesh.points_);
    bounds_ = boundBox(points_, true);
    faces_.swap(otherMesh.faces_);
    owner_.swap(otherMesh.owner_);
    neighbour_.swap(otherMesh.neighbour_);

    // Clear the boundary data
    boundary_.clearGeom();
    boundary_.clearAddressing();
    otherMesh.boundary_.clearGeom();
    otherMesh.boundary_.clearAddressing();

    // Swap the boundaries
    auto updatePatches = []
    (
        const polyPatchList& otherPatches,
        polyBoundaryMesh& boundaryMesh
    )
    {
        boundaryMesh.resize(otherPatches.size());

        forAll(otherPatches, otherPatchi)
        {
            // Clone processor patches, as the decomposition may be different
            // in the other mesh. Just change the size and start of other
            // patches.

            if (isA<processorPolyPatch>(otherPatches[otherPatchi]))
            {
                boundaryMesh.set
                (
                    otherPatchi,
                    otherPatches[otherPatchi].clone(boundaryMesh)
                );
            }
            else
            {
                boundaryMesh[otherPatchi] = polyPatch
                (
                    boundaryMesh[otherPatchi],
                    boundaryMesh,
                    otherPatchi,
                    otherPatches[otherPatchi].size(),
                    otherPatches[otherPatchi].start()
                );
            }
        }
    };

    {
        const polyPatchList patches
        (
            boundary_,
            otherMesh.boundary_
        );
        const polyPatchList otherPatches
        (
            otherMesh.boundary_,
            boundary_
        );

        updatePatches(otherPatches, boundary_);
        updatePatches(patches, otherMesh.boundary_);
    }

    // Parallel data depends on the patch ordering so force recalculation
    globalMeshDataPtr_.clear();
    otherMesh.globalMeshDataPtr_.clear();

    // Flags the mesh files as being changed
    setInstance(time().name());
    otherMesh.setInstance(time().name());

    // Check if the faces and cells are valid
    auto checkFaces = [](const polyMesh& mesh)
    {
        forAll(mesh.faces_, facei)
        {
            const face& curFace = mesh.faces_[facei];

            if (min(curFace) < 0 || max(curFace) > mesh.points_.size())
            {
                FatalErrorInFunction
                    << "Face " << facei << " contains vertex labels out of "
                    << "range: " << curFace << " Max point index = "
                    << mesh.points_.size() << abort(FatalError);
            }
        }
    };

    checkFaces(*this);
    checkFaces(otherMesh);

    // Set the primitive mesh from the owner_, neighbour_.
    // Works out from patch end where the active faces stop.
    initMesh();
    otherMesh.initMesh();

    // Calculate topology for the patches (processor-processor comms etc.)
    boundary_.topoChange();
    otherMesh.boundary_.topoChange();

    // Calculate the geometry for the patches (transformation tensors etc.)
    boundary_.calcGeometry();
    otherMesh.boundary_.calcGeometry();

    // Update the optional pointMesh with respect to the updated polyMesh
    if (foundObject<pointMesh>(pointMesh::typeName))
    {
        pointMesh::New(*this).reset();
    }

    if (otherMesh.foundObject<pointMesh>(pointMesh::typeName))
    {
        pointMesh::New(*this).reset();
    }

    // Swap zones
    pointZones_.swap(otherMesh.pointZones_);
    faceZones_.swap(otherMesh.faceZones_);
    cellZones_.swap(otherMesh.cellZones_);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::polyMesh::~polyMesh()
{
    clearOut();
    resetMotion();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::polyMesh::found(const IOobject& io)
{
    // Create an IO object for the current-time polyMesh directory
    const IOobject curDirIo
    (
        word::null,
        io.time().name(),
        io.name() == polyMesh::defaultRegion
      ? fileName(io.local()/meshSubDir)
      : fileName(io.local()/io.name()/meshSubDir),
        io.time(),
        Foam::IOobject::NO_READ
    );

    // Search back to find the latest-time polyMesh directory
    const IOobject latestDirIo =
        fileHandler().findInstance(curDirIo, io.time().value(), word::null);

    // Return whether or not this latest-time polyMesh directory exists
    return fileHandler().isDir(fileHandler().objectPath(latestDirIo));
}


Foam::fileName Foam::polyMesh::meshDir() const
{
    return dbDir()/meshSubDir;
}


const Foam::fileName& Foam::polyMesh::pointsInstance() const
{
    return points_.instance();
}


const Foam::fileName& Foam::polyMesh::facesInstance() const
{
    return faces_.instance();
}


Foam::IOobject::writeOption Foam::polyMesh::pointsWriteOpt() const
{
    return points_.writeOpt();
}


Foam::IOobject::writeOption Foam::polyMesh::facesWriteOpt() const
{
    return faces_.writeOpt();
}


const Foam::Vector<Foam::label>& Foam::polyMesh::geometricD() const
{
    if (geometricD_.x() == 0)
    {
        calcDirections();
    }

    return geometricD_;
}


Foam::label Foam::polyMesh::nGeometricD() const
{
    return cmptSum(geometricD() + Vector<label>::one)/2;
}


const Foam::Vector<Foam::label>& Foam::polyMesh::solutionD() const
{
    if (solutionD_.x() == 0)
    {
        calcDirections();
    }

    return solutionD_;
}


Foam::label Foam::polyMesh::nSolutionD() const
{
    return cmptSum(solutionD() + Vector<label>::one)/2;
}


const Foam::labelIOList& Foam::polyMesh::tetBasePtIs() const
{
    if (tetBasePtIsPtr_.empty())
    {
        if (debug)
        {
            WarningInFunction
                << "Forcing storage of base points."
                << endl;
        }

        tetBasePtIsPtr_.reset
        (
            new labelIOList
            (
                IOobject
                (
                    "tetBasePtIs",
                    instance(),
                    meshSubDir,
                    *this,
                    IOobject::NO_READ,
                    IOobject::NO_WRITE
                ),
                polyMeshTetDecomposition::findFaceBasePts(*this)
            )
        );
    }

    return tetBasePtIsPtr_();
}


void Foam::polyMesh::addPatches
(
    const List<polyPatch*>& p,
    const bool validBoundary
)
{
    if (boundaryMesh().size())
    {
        FatalErrorInFunction
            << "boundary already exists"
            << abort(FatalError);
    }

    // Reset valid directions
    geometricD_ = Zero;
    solutionD_ = Zero;

    boundary_.setSize(p.size());

    // Copy the patch pointers
    forAll(p, pI)
    {
        boundary_.set(pI, p[pI]);
    }

    // parallelData depends on the processorPatch ordering so force
    // recalculation. Problem: should really be done in removeBoundary but
    // there is some info in parallelData which might be interesting in between
    // removeBoundary and addPatches.
    globalMeshDataPtr_.clear();

    if (validBoundary)
    {
        addedPatches();
    }
}


void Foam::polyMesh::addZones
(
    const List<pointZone*>& pz,
    const List<faceZone*>& fz,
    const List<cellZone*>& cz
)
{
    if (pointZones().size() || faceZones().size() || cellZones().size())
    {
        FatalErrorInFunction
            << "point, face or cell zone already exists"
            << abort(FatalError);
    }

    // Point zones
    if (pz.size())
    {
        pointZones_.setSize(pz.size());

        // Copy the zone pointers
        forAll(pz, pI)
        {
            pointZones_.set(pI, pz[pI]->name(), pz[pI]);
        }

        pointZones_.writeOpt() = IOobject::AUTO_WRITE;
    }

    // Face zones
    if (fz.size())
    {
        faceZones_.setSize(fz.size());

        // Copy the zone pointers
        forAll(fz, fI)
        {
            faceZones_.set(fI, fz[fI]->name(), fz[fI]);
        }

        faceZones_.writeOpt() = IOobject::AUTO_WRITE;
    }

    // Cell zones
    if (cz.size())
    {
        cellZones_.setSize(cz.size());

        // Copy the zone pointers
        forAll(cz, cI)
        {
            cellZones_.set(cI, cz[cI]->name(), cz[cI]);
        }

        cellZones_.writeOpt() = IOobject::AUTO_WRITE;
    }
}


void Foam::polyMesh::reorderPatches
(
    const labelUList& newToOld,
    const bool validBoundary
)
{
    // Clear local fields and e.g. polyMesh parallelInfo
    boundary_.clearGeom();
    clearAddressing(true);

    // Clear all but RepatchableMeshObjects
    meshObjects::clearUpto
    <
        polyMesh,
        DeletableMeshObject,
        RepatchableMeshObject
    >
    (
        *this
    );
    meshObjects::clearUpto
    <
        pointMesh,
        DeletableMeshObject,
        RepatchableMeshObject
    >
    (
        *this
    );

    // Update time instance for the mesh
    // so that it writes the mesh with the changed boundary
    // into a new time directory
    setInstance(time().name());

    boundary_.reorderPatches(newToOld, validBoundary);

    // Warn mesh objects
    meshObjects::reorderPatches<polyMesh>(*this, newToOld, validBoundary);
    meshObjects::reorderPatches<pointMesh>(*this, newToOld, validBoundary);
}


void Foam::polyMesh::addPatch
(
    const label insertPatchi,
    const polyPatch& patch
)
{
    const label sz = boundary_.size();

    label startFacei = nFaces();
    if (insertPatchi < sz)
    {
        startFacei = boundary_[insertPatchi].start();
    }

    // Create reordering list
    // patches before insert position stay as is
    // patches after insert position move one up
    labelList newToOld(boundary_.size()+1);
    for (label i = 0; i < insertPatchi; i++)
    {
        newToOld[i] = i;
    }
    for (label i = insertPatchi; i < sz; i++)
    {
        newToOld[i+1] = i;
    }
    newToOld[insertPatchi] = -1;

    // Reorder
    reorderPatches(newToOld, false);

    // Clear local fields and e.g. polyMesh parallelInfo
    boundary_.clearGeom();
    clearAddressing(true);

    // Clear all but RepatchableMeshObjects
    meshObjects::clearUpto
    <
        polyMesh,
        DeletableMeshObject,
        RepatchableMeshObject
    >
    (
        *this
    );
    meshObjects::clearUpto
    <
        pointMesh,
        DeletableMeshObject,
        RepatchableMeshObject
    >
    (
        *this
    );

    // Insert polyPatch
    boundary_.set
    (
        insertPatchi,
        patch.clone
        (
            boundary_,
            insertPatchi,   // index
            0,              // size
            startFacei      // start
        )
    );

    // Warn mesh objects
    meshObjects::addPatch<polyMesh>(*this, insertPatchi);
    meshObjects::addPatch<pointMesh>(*this, insertPatchi);
}


void Foam::polyMesh::addedPatches()
{
    // Calculate topology for the patches (processor-processor comms etc.)
    boundary_.topoChange();

    // Calculate the geometry for the patches (transformation tensors etc.)
    boundary_.calcGeometry();

    boundary_.checkDefinition();
}


const Foam::pointField& Foam::polyMesh::points() const
{
    if (clearedPrimitives_)
    {
        FatalErrorInFunction
            << "points deallocated"
            << abort(FatalError);
    }

    return points_;
}


const Foam::faceList& Foam::polyMesh::faces() const
{
    if (clearedPrimitives_)
    {
        FatalErrorInFunction
            << "faces deallocated"
            << abort(FatalError);
    }

    return faces_;
}


const Foam::labelList& Foam::polyMesh::faceOwner() const
{
    return owner_;
}


const Foam::labelList& Foam::polyMesh::faceNeighbour() const
{
    return neighbour_;
}


const Foam::pointField& Foam::polyMesh::oldPoints() const
{
    if (!moving_)
    {
        return points_;
    }

    if (oldPointsPtr_.empty())
    {
        FatalErrorInFunction
            << "Old points have not been stored"
            << exit(FatalError);
    }

    return oldPointsPtr_();
}


const Foam::pointField& Foam::polyMesh::oldCellCentres() const
{
    storeOldCellCentres_ = true;

    if (!moving_)
    {
        return cellCentres();
    }

    if (oldCellCentresPtr_.empty())
    {
        FatalErrorInFunction
            << "Old cell centres have not been stored"
            << exit(FatalError);
    }

    return oldCellCentresPtr_();
}


void Foam::polyMesh::setPoints(const pointField& newPoints)
{
    if (debug)
    {
        InfoInFunction
            << "Set points for time " << time().value()
            << " index " << time().timeIndex() << endl;
    }

    primitiveMesh::clearGeom();

    points_ = newPoints;

    setPointsInstance(time().name());

    // Adjust parallel shared points
    if (globalMeshDataPtr_.valid())
    {
        globalMeshDataPtr_().movePoints(points_);
    }

    // Force recalculation of all geometric data with new points

    bounds_ = boundBox(points_);
    boundary_.movePoints(points_);

    pointZones_.movePoints(points_);
    faceZones_.movePoints(points_);
    cellZones_.movePoints(points_);

    // Reset valid directions (could change with rotation)
    geometricD_ = Zero;
    solutionD_ = Zero;

    meshObjects::movePoints<polyMesh>(*this);
    meshObjects::movePoints<pointMesh>(*this);
}


Foam::tmp<Foam::scalarField> Foam::polyMesh::movePoints
(
    const pointField& newPoints
)
{
    if (debug)
    {
        InfoInFunction
            << "Moving points for time " << time().value()
            << " index " << time().timeIndex() << endl;
    }

    // Pick up old points and cell centres
    if (curMotionTimeIndex_ != time().timeIndex())
    {
        oldPointsPtr_.clear();
        oldPointsPtr_.reset(new pointField(points_));
        if (storeOldCellCentres_)
        {
            oldCellCentresPtr_.clear();
            oldCellCentresPtr_.reset(new pointField(cellCentres()));
        }
        curMotionTimeIndex_ = time().timeIndex();
    }

    points_ = newPoints;

    setPointsInstance(time().name());

    tmp<scalarField> sweptVols = primitiveMesh::movePoints
    (
        points_,
        oldPoints()
    );

    // Adjust parallel shared points
    if (globalMeshDataPtr_.valid())
    {
        globalMeshDataPtr_().movePoints(points_);
    }

    // Force recalculation of all geometric data with new points

    bounds_ = boundBox(points_);
    boundary_.movePoints(points_);

    pointZones_.movePoints(points_);
    faceZones_.movePoints(points_);
    cellZones_.movePoints(points_);

    // Reset valid directions (could change with rotation)
    geometricD_ = Zero;
    solutionD_ = Zero;

    meshObjects::movePoints<polyMesh>(*this);
    meshObjects::movePoints<pointMesh>(*this);

    return sweptVols;
}


void Foam::polyMesh::resetMotion() const
{
    curMotionTimeIndex_ = -1;
    oldPointsPtr_.clear();
    oldCellCentresPtr_.clear();
}


const Foam::globalMeshData& Foam::polyMesh::globalData() const
{
    if (globalMeshDataPtr_.empty())
    {
        if (debug)
        {
            Pout<< "polyMesh::globalData() const : "
                << "Constructing parallelData from processor topology"
                << endl;
        }
        // Construct globalMeshData using processorPatch information only.
        globalMeshDataPtr_.reset(new globalMeshData(*this));
    }

    return globalMeshDataPtr_();
}


Foam::label Foam::polyMesh::comm() const
{
    return comm_;
}


Foam::label& Foam::polyMesh::comm()
{
    return comm_;
}


void Foam::polyMesh::removeFiles(const fileName& instanceDir) const
{
    fileName meshFilesPath = thisDb().time().path()/instanceDir/meshDir();

    rm(meshFilesPath/"points");
    rm(meshFilesPath/"faces");
    rm(meshFilesPath/"owner");
    rm(meshFilesPath/"neighbour");
    rm(meshFilesPath/"cells");
    rm(meshFilesPath/"boundary");
    rm(meshFilesPath/"pointZones");
    rm(meshFilesPath/"faceZones");
    rm(meshFilesPath/"cellZones");
    rm(meshFilesPath/"meshModifiers");
    rm(meshFilesPath/"parallelData");

    // remove subdirectories
    if (isDir(meshFilesPath/"sets"))
    {
        rmDir(meshFilesPath/"sets");
    }
}


void Foam::polyMesh::removeFiles() const
{
    removeFiles(instance());
}


// ************************************************************************* //
