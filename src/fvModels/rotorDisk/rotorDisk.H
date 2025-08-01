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
    Foam::fv::rotorDisk

Description
    Cell based momentum source which approximates the mean effects of
    rotor forces on a cylindrical region within the domain.

Usage
    Example usage:
    \verbatim
    rotorDisk1
    {
        type            rotorDisk;

        U               U;          // Name of the velocity field

        nBlades         3;          // Number of blades
        tipEffect       0.96;       // Normalised radius above which lift = 0
        inletFlowType   local;      // Inlet flow type specification
        geometryMode    auto;       // Geometry specification
        refDirection    (-1 0 0);   // Reference direction for psi angle

        trimModel       fixed;      // Trim model; fixed or targetCoeff
        // see fixedTrim.H or targetCoeffTrim.H for documentation

        flapCoeffs
        {
            beta0           0;  // Coning angle [deg]
            beta1c          0;  // Lateral flapping coeff (cos coeff)
            beta2s          0;  // Longitudinal flapping coeff (sin coeff)
        }

        blade
        {
            // see bladeModel.H for documentation
        }

        profiles
        {
            profile1
            {
                type    lookup;     // Profile model; lookup or series

                ...

                // see lookupProfile.H or seriesProfile.H for documentation
            }
            profile2
            {
                ...
            }
        }
    }
    \endverbatim

    Where:
    Valid fvModels for the \c geometryMode entry include:
    - auto          : determine rotor coordinate system from cells
    - specified     : specified coordinate system

    Valid fvModels for the \c inletFlowType entry include:
    - fixed         : specified velocity
    - local         : use local flow conditions
    - surfaceNormal : specified normal velocity (positive towards rotor)

See also
    Foam::bladeModel
    Foam::lookupProfile
    Foam::seriesProfile

SourceFiles
    rotorDisk.C
    rotorDiskTemplates.C

\*---------------------------------------------------------------------------*/

#ifndef rotorDisk_H
#define rotorDisk_H

#include "fvModel.H"
#include "fvCellZone.H"
#include "cylindricalCS.H"
#include "cylindrical.H"
#include "NamedEnum.H"
#include "bladeModel.H"
#include "profileModelList.H"
#include "trimModel.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{

/*---------------------------------------------------------------------------*\
                       Class rotorDisk Declaration
\*---------------------------------------------------------------------------*/

class rotorDisk
:
    public fvModel
{
public:

    enum geometryModeType
    {
        automatic,
        specified
    };
    static const NamedEnum<geometryModeType, 2> geometryModeTypeNames_;

    enum inletFlowType
    {
        fixed,
        surfaceNormal,
        local
    };
    static const NamedEnum<inletFlowType, 3> inletFlowTypeNames_;


private:

    // Helper structures to encapsulate flap and trim data
    // Note: all input in degrees (converted to radians internally)

        struct flapData
        {
            scalar beta0;   // coning angle
            scalar beta1c;  // lateral flapping coeff (cos coeff)
            scalar beta2s;  // longitudinal flapping coeff (sin coeff)
        };


    // Private data

        //- The cellZone the fvModel applies to
        fvCellZone zone_;

        //- Name of the velocity field
        word UName_;

        //- Rotational speed [rad/s]
        //  Positive anti-clockwise when looking along -ve lift direction
        scalar omega_;

        //- Number of blades
        label nBlades_;

        //- Inlet flow type
        inletFlowType inletFlow_;

        //- Inlet velocity for specified inflow
        vector inletVelocity_;

        //- Tip effect [0-1]
        //  Ratio of blade radius beyond which lift=0
        scalar tipEffect_;

        //- Blade flap coefficients [rad/s]
        flapData flap_;

        //- Cell centre positions in local rotor frame
        //  (Cylindrical r, theta, z)
        List<point> x_;

        //- Rotation tensor for flap angle
        List<tensor> R_;

        //- Inverse rotation tensor for flap angle
        List<tensor> invR_;

        //- Area [m^2]
        List<scalar> area_;

        //- Rotor local cylindrical co-ordinate system (r, theta, z)
        coordinateSystems::cylindrical coordSys_;

         //- Rotor transformation co-ordinate system
        autoPtr<cylindrical> cylindrical_;

        //- Maximum radius
        scalar rMax_;

        //- Trim model
        autoPtr<trimModel> trim_;

        //- Blade data
        bladeModel blade_;

        //- Profile data
        profileModelList profiles_;

        //- Reference density for incompressible case
        mutable scalar rhoRef_;


    // Protected Member Functions

        //- Non-virtual read
        void readCoeffs(const dictionary& dict);

        //- Check data
        void checkData(const dictionary& dict);

        //- Set the face areas per cell, and optionally correct the rotor axis
        void setFaceArea(vector& axis, const bool correct);

        //- Create the co-ordinate system
        void createCoordinateSystem(const dictionary& dict);

        //- Construct geometry
        void constructGeometry();

        //- Return the inlet flow field
        tmp<vectorField> inflowVelocity(const volVectorField& U) const;

        //- Helper function to write rotor data
        template<class Type>
        void writeField(const word& name, const List<Type>& values) const;


public:

    //- Runtime type information
    TypeName("rotorDisk");


    // Constructors


        //- Construct from components
        rotorDisk
        (
            const word& name,
            const word& modelType,
            const fvMesh& mesh,
            const dictionary& dict
        );


    //- Destructor
    virtual ~rotorDisk();


    // Member Functions

        // Access

            //- Return the reference density for incompressible case
            inline scalar rhoRef() const;

            //- Return the rotational speed [rad/s]
            //  Positive anti-clockwise when looking along -ve lift direction
            inline scalar omega() const;

            inline const fvCellZone& zone() const;

            //- Return the cell centre positions in local rotor frame
            // (Cylindrical r, theta, z)
            inline const List<point>& x() const;

            //- Return the rotor co-ordinate system (r, theta, z)
            inline const coordinateSystems::cylindrical& coordSys() const;


        // Checks

            //- Return the list of fields for which the fvModel adds source term
            //  to the transport equation
            virtual wordList addSupFields() const;


        // Evaluation

            //- Calculate forces
            template<class RhoFieldType>
            void calculate
            (
                const RhoFieldType& rho,
                const vectorField& U,
                const scalarField& thetag,
                vectorField& force,
                const bool divideVolume = true,
                const bool output = true
            ) const;


        // Source term addition

            //- Source term to momentum equation
            virtual void addSup
            (
                const volVectorField& U,
                fvMatrix<vector>& eqn
            ) const;

            //- Source term to compressible momentum equation
            virtual void addSup
            (
                const volScalarField& rho,
                const volVectorField& U,
                fvMatrix<vector>& eqn
            ) const;


        // Mesh changes

            //- Update for mesh motion
            virtual bool movePoints();

            //- Update topology using the given map
            virtual void topoChange(const polyTopoChangeMap&);

            //- Update from another mesh using the given map
            virtual void mapMesh(const polyMeshMap&);

            //- Redistribute or update using the given distribution map
            virtual void distribute(const polyDistributionMap&);


        // IO

            //- Read source dictionary
            virtual bool read(const dictionary& dict);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "rotorDiskI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "rotorDiskTemplates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
