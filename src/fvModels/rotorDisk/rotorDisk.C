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

#include "rotorDisk.H"
#include "fvMatrices.H"
#include "geometricOneField.H"
#include "syncTools.H"
#include "axesRotation.H"
#include "omega.H"
#include "addToRunTimeSelectionTable.H"

using namespace Foam::constant;

// * * * * * * * * * * * * * Static Member Functions * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{
    defineTypeNameAndDebug(rotorDisk, 0);
    addToRunTimeSelectionTable(fvModel, rotorDisk, dictionary);
    addBackwardCompatibleToRunTimeSelectionTable
    (
        fvModel,
        rotorDisk,
        dictionary,
        rotorDiskSource,
        "rotorDiskSource"
    );
}
}


const Foam::NamedEnum<Foam::fv::rotorDisk::geometryModeType, 2>
Foam::fv::rotorDisk::geometryModeTypeNames_
{
    "auto",
    "specified"
};

const Foam::NamedEnum<Foam::fv::rotorDisk::inletFlowType, 3>
Foam::fv::rotorDisk::inletFlowTypeNames_
{
    "fixed",
    "surfaceNormal",
    "local"
};


// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

void Foam::fv::rotorDisk::readCoeffs(const dictionary& dict)
{
    UName_ = dict.lookupOrDefault<word>("U", "U");

    omega_ = Foam::omega(coeffs(dict)).value();

    dict.lookup("nBlades") >> nBlades_;

    inletFlow_ = inletFlowTypeNames_.read(dict.lookup("inletFlowType"));

    dict.lookup("tipEffect") >> tipEffect_;

    const dictionary& flapCoeffs(dict.subDict("flapCoeffs"));
    flap_.beta0 = flapCoeffs.lookup<scalar>("beta0", unitDegrees);
    flap_.beta1c = flapCoeffs.lookup<scalar>("beta1c", unitDegrees);
    flap_.beta2s = flapCoeffs.lookup<scalar>("beta2s", unitDegrees);

    // Create co-ordinate system
    createCoordinateSystem(dict);

    // Read co-odinate system dependent properties
    checkData(dict);

    constructGeometry();

    trim_->read(coeffs(dict));

    if (debug)
    {
        writeField("thetag", trim_->thetag()());
        writeField("faceArea", area_);
    }

    // Optionally read the reference density for incompressible flow
    if (dict.found("rhoRef"))
    {
        dict.lookup("rhoRef") >> rhoRef_;
    }
}


void Foam::fv::rotorDisk::checkData(const dictionary& dict)
{
    // Set the profile ID for each blade section
    profiles_.connectBlades
    (
        blade_.profileName(),
        blade_.profileIndex()
    );
    switch (inletFlow_)
    {
        case inletFlowType::fixed:
        {
            dict.lookup("inletVelocity") >> inletVelocity_;
            break;
        }
        case inletFlowType::surfaceNormal:
        {
            scalar UIn
            (
                dict.lookup<scalar>("inletNormalVelocity")
            );
            inletVelocity_ = -coordSys_.R().e3()*UIn;
            break;
        }
        case inletFlowType::local:
        {
            break;
        }
        default:
        {
            FatalErrorInFunction
                << "Unknown inlet velocity type" << abort(FatalError);
        }
    }
}


void Foam::fv::rotorDisk::setFaceArea(vector& axis, const bool correct)
{
    area_ = 0.0;

    static const scalar tol = 0.8;

    const label nInternalFaces = mesh().nInternalFaces();
    const polyBoundaryMesh& pbm = mesh().boundaryMesh();
    const vectorField& Sf = mesh().Sf();
    const scalarField& magSf = mesh().magSf();

    vector n = Zero;

    // Calculate cell addressing for selected cells
    labelList cellAddr(mesh().nCells(), -1);
    UIndirectList<label>(cellAddr, zone_.zone()) =
        identityMap(zone_.nCells());
    labelList nbrFaceCellAddr(mesh().nFaces() - nInternalFaces, -1);
    forAll(pbm, patchi)
    {
        const polyPatch& pp = pbm[patchi];

        if (pp.coupled())
        {
            forAll(pp, i)
            {
                label facei = pp.start() + i;
                label nbrFacei = facei - nInternalFaces;
                label own = mesh().faceOwner()[facei];
                nbrFaceCellAddr[nbrFacei] = cellAddr[own];
            }
        }
    }

    // Correct for parallel running
    syncTools::swapBoundaryFaceList(mesh(), nbrFaceCellAddr);

    // Add internal field contributions
    for (label facei = 0; facei < nInternalFaces; facei++)
    {
        const label own = cellAddr[mesh().faceOwner()[facei]];
        const label nbr = cellAddr[mesh().faceNeighbour()[facei]];

        if ((own != -1) && (nbr == -1))
        {
            vector nf = Sf[facei]/magSf[facei];

            if ((nf & axis) > tol)
            {
                area_[own] += magSf[facei];
                n += Sf[facei];
            }
        }
        else if ((own == -1) && (nbr != -1))
        {
            vector nf = Sf[facei]/magSf[facei];

            if ((-nf & axis) > tol)
            {
                area_[nbr] += magSf[facei];
                n -= Sf[facei];
            }
        }
    }


    // Add boundary contributions
    forAll(pbm, patchi)
    {
        const polyPatch& pp = pbm[patchi];
        const vectorField& Sfp = mesh().Sf().boundaryField()[patchi];
        const scalarField& magSfp = mesh().magSf().boundaryField()[patchi];

        if (pp.coupled())
        {
            forAll(pp, j)
            {
                const label facei = pp.start() + j;
                const label own = cellAddr[mesh().faceOwner()[facei]];
                const label nbr = nbrFaceCellAddr[facei - nInternalFaces];
                const vector nf = Sfp[j]/magSfp[j];

                if ((own != -1) && (nbr == -1) && ((nf & axis) > tol))
                {
                    area_[own] += magSfp[j];
                    n += Sfp[j];
                }
            }
        }
        else
        {
            forAll(pp, j)
            {
                const label facei = pp.start() + j;
                const label own = cellAddr[mesh().faceOwner()[facei]];
                const vector nf = Sfp[j]/magSfp[j];

                if ((own != -1) && ((nf & axis) > tol))
                {
                    area_[own] += magSfp[j];
                    n += Sfp[j];
                }
            }
        }
    }

    if (correct)
    {
        reduce(n, sumOp<vector>());
        axis = n/mag(n);
    }

    if (debug)
    {
        volScalarField area
        (
            IOobject
            (
                name() + ":area",
                mesh().time().name(),
                mesh(),
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh(),
            dimensionedScalar(dimArea, 0)
        );
        UIndirectList<scalar>(area.primitiveField(), zone_.zone()) = area_;

        Info<< type() << ": " << name() << " writing field " << area.name()
            << endl;

        area.write();
    }
}


void Foam::fv::rotorDisk::createCoordinateSystem(const dictionary& dict)
{
    // Construct the local rotor co-prdinate system
    vector origin(Zero);
    vector axis(Zero);
    vector refDir(Zero);

    geometryModeType gm =
        geometryModeTypeNames_.read(dict.lookup("geometryMode"));

    switch (gm)
    {
        case geometryModeType::automatic:
        {
            // Determine rotation origin (cell volume weighted)
            scalar sumV = 0.0;
            const scalarField& V = mesh().V();
            const vectorField& C = mesh().C();

            const labelList& cells = zone_.zone();

            forAll(cells, i)
            {
                const label celli = cells[i];
                sumV += V[celli];
                origin += V[celli]*C[celli];
            }
            reduce(origin, sumOp<vector>());
            reduce(sumV, sumOp<scalar>());
            origin /= sumV;

            // Determine first radial vector
            vector dx1(Zero);
            scalar magR = -great;
            forAll(cells, i)
            {
                const label celli = cells[i];
                vector test = C[celli] - origin;
                if (mag(test) > magR)
                {
                    dx1 = test;
                    magR = mag(test);
                }
            }
            reduce(dx1, maxMagSqrOp<vector>());
            magR = mag(dx1);

            // Determine second radial vector and cross to determine axis
            forAll(cells, i)
            {
                const label celli = cells[i];
                vector dx2 = C[celli] - origin;
                if (mag(dx2) > 0.5*magR)
                {
                    axis = dx1 ^ dx2;
                    if (mag(axis) > small)
                    {
                        break;
                    }
                }
            }
            reduce(axis, maxMagSqrOp<vector>());
            axis /= mag(axis);

            // Correct the axis direction using a point above the rotor
            {
                vector pointAbove(dict.lookup("pointAbove"));
                vector dir = pointAbove - origin;
                dir /= mag(dir);
                if ((dir & axis) < 0)
                {
                    axis *= -1.0;
                }
            }

            dict.lookup("refDirection") >> refDir;

            cylindrical_.reset
            (
                new cylindrical(axis, origin, UIndirectList<vector>(C, cells)())
            );

            // Set the face areas and apply correction to calculated axis
            // e.g. if cellZone is more than a single layer in thickness
            setFaceArea(axis, true);

            break;
        }
        case geometryModeType::specified:
        {
            dict.lookup("origin") >> origin;
            dict.lookup("axis") >> axis;
            dict.lookup("refDirection") >> refDir;

            cylindrical_.reset
            (
                new cylindrical
                (
                    axis,
                    origin,
                    UIndirectList<vector>(mesh().C(), zone_.zone())()
                )
            );

            setFaceArea(axis, false);

            break;
        }
    }

    coordSys_ =
        coordinateSystems::cylindrical("rotorCoordSys", origin, axis, refDir);

    const scalar sumArea = gSum(area_);
    const scalar diameter = Foam::sqrt(4.0*sumArea/mathematical::pi);
    Info<< "    Rotor geometry:" << nl
        << "    - disk diameter = " << diameter << nl
        << "    - disk area     = " << sumArea << nl
        << "    - origin        = " << coordSys_.origin() << nl
        << "    - r-axis        = " << coordSys_.R().e1() << nl
        << "    - psi-axis      = " << coordSys_.R().e2() << nl
        << "    - z-axis        = " << coordSys_.R().e3() << endl;
}


void Foam::fv::rotorDisk::constructGeometry()
{
    const vectorField& C = mesh().C();

    const labelList& cells = zone_.zone();

    forAll(cells, i)
    {
        if (area_[i] > rootVSmall)
        {
            const label celli = cells[i];

            // Position in (planar) rotor co-ordinate system
            x_[i] = coordSys_.localPosition(C[celli]);

            // Cache max radius
            rMax_ = max(rMax_, x_[i].x());

            // Swept angle relative to rDir axis [radians] in range 0 -> 2*pi
            scalar psi = x_[i].y();

            // Blade flap angle [radians]
            scalar beta =
                flap_.beta0 - flap_.beta1c*cos(psi) - flap_.beta2s*sin(psi);

            // Determine rotation tensor to convert from planar system into the
            // rotor cone system
            scalar c = cos(beta);
            scalar s = sin(beta);
            R_[i] = tensor(c, 0, -s, 0, 1, 0, s, 0, c);
            invR_[i] = R_[i].T();
        }
    }
}


Foam::tmp<Foam::vectorField> Foam::fv::rotorDisk::inflowVelocity
(
    const volVectorField& U
) const
{
    switch (inletFlow_)
    {
        case inletFlowType::fixed:
        case inletFlowType::surfaceNormal:
        {
            return tmp<vectorField>
            (
                new vectorField(mesh().nCells(), inletVelocity_)
            );

            break;
        }
        case inletFlowType::local:
        {
            return U.primitiveField();

            break;
        }
        default:
        {
            FatalErrorInFunction
                << "Unknown inlet flow specification" << abort(FatalError);
        }
    }

    return tmp<vectorField>(new vectorField(mesh().nCells(), Zero));
}


// * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * * //

Foam::fv::rotorDisk::rotorDisk
(
    const word& name,
    const word& modelType,
    const fvMesh& mesh,
    const dictionary& dict
)
:
    fvModel(name, modelType, mesh, dict),
    zone_(mesh, dict),
    UName_(word::null),
    omega_(0),
    nBlades_(0),
    inletFlow_(inletFlowType::local),
    inletVelocity_(Zero),
    tipEffect_(1),
    flap_(),
    x_(zone_.nCells(), Zero),
    R_(zone_.nCells(), I),
    invR_(zone_.nCells(), I),
    area_(zone_.nCells(), Zero),
    coordSys_("rotorCoordSys", vector::zero, axesRotation(sphericalTensor::I)),
    cylindrical_(),
    rMax_(0),
    trim_(trimModel::New(*this, dict)),
    blade_(dict.subDict("blade")),
    profiles_(dict.subDict("profiles")),
    rhoRef_(-1)
{
    readCoeffs(coeffs(dict));
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::fv::rotorDisk::~rotorDisk()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::wordList Foam::fv::rotorDisk::addSupFields() const
{
    return wordList(1, UName_);
}


void Foam::fv::rotorDisk::addSup
(
    const volVectorField& U,
    fvMatrix<vector>& eqn
) const
{
    volVectorField::Internal force
    (
        IOobject
        (
            name() + ":rotorForce",
            mesh().time().name(),
            mesh()
        ),
        mesh(),
        dimensionedVector
        (
            "zero",
            eqn.dimensions()/dimVolume,
            Zero
        )
    );

    if (rhoRef_ < 0)
    {
        FatalError
            << "rhoRef not set, required for incompressible flow"
            << exit(FatalError);
    }

    const vectorField Uin(inflowVelocity(U));
    trim_->correct(Uin, force);
    calculate(geometricOneField(), Uin, trim_->thetag(), force);

    // Add source to rhs of eqn
    eqn -= force;

    if (mesh().time().writeTime())
    {
        force.write();
    }
}


void Foam::fv::rotorDisk::addSup
(
    const volScalarField& rho,
    const volVectorField& U,
    fvMatrix<vector>& eqn
) const
{
    volVectorField::Internal force
    (
        IOobject
        (
            name() + ":rotorForce",
            mesh().time().name(),
            mesh()
        ),
        mesh(),
        dimensionedVector
        (
            "zero",
            eqn.dimensions()/dimVolume,
            Zero
        )
    );

    const vectorField Uin(inflowVelocity(U));
    trim_->correct(rho, Uin, force);
    calculate(rho, Uin, trim_->thetag(), force);

    // Add source to rhs of eqn
    eqn -= force;

    if (mesh().time().writeTime())
    {
        force.write();
    }
}


bool Foam::fv::rotorDisk::movePoints()
{
    zone_.movePoints();
    return true;
}


void Foam::fv::rotorDisk::topoChange(const polyTopoChangeMap& map)
{
    zone_.topoChange(map);
}


void Foam::fv::rotorDisk::mapMesh(const polyMeshMap& map)
{
    zone_.mapMesh(map);
}


void Foam::fv::rotorDisk::distribute(const polyDistributionMap& map)
{
    zone_.distribute(map);
}


bool Foam::fv::rotorDisk::read(const dictionary& dict)
{
    if (fvModel::read(dict))
    {
        zone_.read(coeffs(dict));
        readCoeffs(coeffs(dict));
        return true;
    }
    else
    {
        return false;
    }
}


// ************************************************************************* //
