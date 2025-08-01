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
    Foam::MomentumLookupTableInjection

Description
    Particle injection sources read from look-up table. Each row corresponds to
    an injection site.

    \verbatim
    (
        (x y z) (u v w) d rho mDot   // injector 1
        (x y z) (u v w) d rho mDot   // injector 2
        ...
        (x y z) (u v w) d rho mDot   // injector N
    );
    \endverbatim

    where:
    \plaintable
        x, y, z | global cartesian co-ordinates [m]
        u, v, w | global cartesian velocity components [m/s]
        d       | diameter [m]
        rho     | density [kg/m^3]
        mDot    | mass flow rate [kg/s]
    \endplaintable

SourceFiles
    MomentumLookupTableInjection.C

\*---------------------------------------------------------------------------*/

#ifndef MomentumLookupTableInjection_H
#define MomentumLookupTableInjection_H

#include "InjectionModel.H"
#include "momentumParcelInjectionDataIOList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                Class MomentumLookupTableInjection Declaration
\*---------------------------------------------------------------------------*/

template<class CloudType>
class MomentumLookupTableInjection
:
    public InjectionModel<CloudType>
{
    // Private Data

        //- Name of file containing injector/parcel data
        const word inputFileName_;

        //- Injection duration - common to all injection sources
        const scalar duration_;

        //- Number of parcels per injector - common to all injection sources
        autoPtr<Function1<scalar>> parcelsPerSecond_;

        //- Flag to indicate to randomise injection positions
        bool randomise_;

        //- List of injectors
        momentumParcelInjectionDataIOList injectors_;

        //- List of coordinates corresponding to injector positions
        List<barycentric> injectorCoordinates_;

        //- List of cell labels corresponding to injector positions
        labelList injectorCells_;

        //- List of tetFace labels corresponding to injector positions
        labelList injectorTetFaces_;

        //- List of tetPt labels corresponding to injector positions
        labelList injectorTetPts_;


public:

    //- Runtime type information
    TypeName("momentumLookupTableInjection");


    // Constructors

        //- Construct from dictionary
        MomentumLookupTableInjection
        (
            const dictionary& dict,
            CloudType& owner,
            const word& modelName
        );

        //- Construct copy
        MomentumLookupTableInjection
        (
            const MomentumLookupTableInjection<CloudType>& im
        );

        //- Construct and return a clone
        virtual autoPtr<InjectionModel<CloudType>> clone() const
        {
            return autoPtr<InjectionModel<CloudType>>
            (
                new MomentumLookupTableInjection<CloudType>(*this)
            );
        }


    //- Destructor
    virtual ~MomentumLookupTableInjection();


    // Member Functions

        //- Set injector locations when mesh is updated
        virtual void topoChange();

        //- Return the end-of-injection time
        virtual scalar timeEnd() const;

        //- Number of parcels to introduce relative to SOI
        virtual scalar nParcelsToInject(const scalar time0, const scalar time1);

        //- Parcel mass to introduce relative to SOI
        virtual scalar massToInject(const scalar time0, const scalar time1);


        // Injection geometry

            //- Set the injection position and owner cell, tetFace and tetPt
            virtual void setPositionAndCell
            (
                const meshSearch& searchEngine,
                const label parcelI,
                const label nParcels,
                const scalar time,
                barycentric& coordinates,
                label& celli,
                label& tetFacei,
                label& tetPti,
                label& facei
            );

            //- Set the parcel properties
            virtual void setProperties
            (
                const label parcelI,
                const label nParcels,
                const scalar time,
                typename CloudType::parcelType::trackingData& td,
                typename CloudType::parcelType& parcel
            );

            //- Flag to identify whether model fully describes the parcel
            virtual bool fullyDescribed() const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "MomentumLookupTableInjection.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
