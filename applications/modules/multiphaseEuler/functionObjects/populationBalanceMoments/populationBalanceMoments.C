/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2022-2025 OpenFOAM Foundation
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

#include "populationBalanceMoments.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
    defineTypeNameAndDebug(populationBalanceMoments, 0);
    addToRunTimeSelectionTable
    (
        functionObject,
        populationBalanceMoments,
        dictionary
    );
}
}

const Foam::NamedEnum
<
    Foam::functionObjects::populationBalanceMoments::momentType,
    4
>
Foam::functionObjects::populationBalanceMoments::momentTypeNames_
{"integerMoment", "mean", "variance", "stdDev"};

const Foam::NamedEnum
<
    Foam::functionObjects::populationBalanceMoments::coordinateType,
    3
>
Foam::functionObjects::populationBalanceMoments::coordinateTypeNames_
{"volume", "area", "diameter"};

const Foam::NamedEnum
<
    Foam::functionObjects::populationBalanceMoments::weightType,
    3
>
Foam::functionObjects::populationBalanceMoments::weightTypeNames_
{
    "numberConcentration",
    "volumeConcentration",
    "areaConcentration"
};

const Foam::NamedEnum
<
    Foam::functionObjects::populationBalanceMoments::meanType,
    3
>
Foam::functionObjects::populationBalanceMoments::meanTypeNames_
{"arithmetic", "geometric", "notApplicable"};


// * * * * * * * * * * * * Private Member Functions * * * * * * * * * * * * //

Foam::word
Foam::functionObjects::populationBalanceMoments::coordinateTypeSymbolicName()
{
    word coordinateTypeSymbolicName(word::null);

    switch (coordinateType_)
    {
        case coordinateType::volume:
        {
            coordinateTypeSymbolicName = "v";

            break;
        }
        case coordinateType::area:
        {
            coordinateTypeSymbolicName = "a";

            break;
        }
        case coordinateType::diameter:
        {
            coordinateTypeSymbolicName = "d";

            break;
        }
    }

    return coordinateTypeSymbolicName;
}


Foam::word
Foam::functionObjects::populationBalanceMoments::weightTypeSymbolicName()
{
    word weightTypeSymbolicName(word::null);

    switch (weightType_)
    {
        case weightType::numberConcentration:
        {
            weightTypeSymbolicName = "N";

            break;
        }
        case weightType::volumeConcentration:
        {
            weightTypeSymbolicName = "V";

            break;
        }
        case weightType::areaConcentration:
        {
            weightTypeSymbolicName = "A";

            break;
        }
    }

    return weightTypeSymbolicName;
}


Foam::word Foam::functionObjects::populationBalanceMoments::defaultFldName()
{
    word meanName
    (
        meanType_ == meanType::geometric
      ? word(meanTypeNames_[meanType_]).capitalise()
      : word("")
    );

    return
        word
        (
            IOobject::groupName
            (
                "weighted"
              + meanName
              + word(momentTypeNames_[momentType_]).capitalise()
              + "("
              + weightTypeSymbolicName()
              + ","
              + coordinateTypeSymbolicName()
              + ")",
                popBalName_
            )
        );
}


Foam::word
Foam::functionObjects::populationBalanceMoments::integerMomentFldName()
{
    return
        word
        (
            IOobject::groupName
            (
                word(momentTypeNames_[momentType_])
              + Foam::name(order_)
              + "("
              + weightTypeSymbolicName()
              + ","
              + coordinateTypeSymbolicName()
              + ")",
                popBalName_
            )
        );
}


void Foam::functionObjects::populationBalanceMoments::setDimensions
(
    volScalarField& fld,
    momentType momType
)
{
    switch (momType)
    {
        case momentType::integerMoment:
        {
            switch (coordinateType_)
            {
                case coordinateType::volume:
                {
                    fld.dimensions().reset
                    (
                        pow(dimVolume, order_)/dimVolume
                    );

                    break;
                }
                case coordinateType::area:
                {
                    fld.dimensions().reset
                    (
                        pow(dimArea, order_)/dimVolume
                    );

                    break;
                }
                case coordinateType::diameter:
                {
                    fld.dimensions().reset
                    (
                        pow(dimLength, order_)/dimVolume
                    );

                    break;
                }
            }

            switch (weightType_)
            {
                case weightType::volumeConcentration:
                {
                    fld.dimensions().reset(fld.dimensions()*dimVolume);

                    break;
                }
                case weightType::areaConcentration:
                {
                    fld.dimensions().reset(fld.dimensions()*dimArea);

                    break;
                }
                default:
                {
                    break;
                }
            }

            break;
        }
        case momentType::mean:
        {
            switch (coordinateType_)
            {
                case coordinateType::volume:
                {
                    fld.dimensions().reset(dimVolume);

                    break;
                }
                case coordinateType::area:
                {
                    fld.dimensions().reset(dimArea);

                    break;
                }
                case coordinateType::diameter:
                {
                    fld.dimensions().reset(dimLength);

                    break;
                }
            }

            break;
        }
        case momentType::variance:
        {
            switch (coordinateType_)
            {
                case coordinateType::volume:
                {
                    fld.dimensions().reset(sqr(dimVolume));

                    break;
                }
                case coordinateType::area:
                {
                    fld.dimensions().reset(sqr(dimArea));

                    break;
                }
                case coordinateType::diameter:
                {
                    fld.dimensions().reset(sqr(dimLength));

                    break;
                }
            }

            if (meanType_ == meanType::geometric)
            {
                fld.dimensions().reset(dimless);
            }

            break;
        }
        case momentType::stdDev:
        {
            switch (coordinateType_)
            {
                case coordinateType::volume:
                {
                    fld.dimensions().reset(dimVolume);

                    break;
                }
                case coordinateType::area:
                {
                    fld.dimensions().reset(dimArea);

                    break;
                }
                case coordinateType::diameter:
                {
                    fld.dimensions().reset(dimLength);

                    break;
                }
            }

            if (meanType_ == meanType::geometric)
            {
                fld.dimensions().reset(dimless);
            }

            break;
        }
    }
}


Foam::tmp<Foam::volScalarField>
Foam::functionObjects::populationBalanceMoments::totalConcentration
(
    const populationBalanceModel& popBal
)
{
    tmp<volScalarField> tTotalConcentration
    (
        volScalarField::New
        (
            "totalConcentration",
            mesh_,
            dimensionedScalar(inv(dimVolume), Zero)
        )
    );

    volScalarField& totalConcentration = tTotalConcentration.ref();

    switch (weightType_)
    {
        case weightType::volumeConcentration:
        {
            totalConcentration.dimensions().reset
            (
                totalConcentration.dimensions()*dimVolume
            );

            break;
        }
        case weightType::areaConcentration:
        {
            totalConcentration.dimensions().reset
            (
                totalConcentration.dimensions()*dimArea
            );

            break;
        }
        default:
        {
            break;
        }
    }

    forAll(popBal.fs(), i)
    {
        const volScalarField& alpha = popBal.phases()[i];
        const volScalarField& fi = popBal.f(i);
        const dimensionedScalar& vi = popBal.v(i);

        switch (weightType_)
        {
            case weightType::numberConcentration:
            {
                totalConcentration += fi*alpha/vi;

                break;
            }
            case weightType::volumeConcentration:
            {
                totalConcentration += fi*alpha;

                break;
            }
            case weightType::areaConcentration:
            {
                totalConcentration += popBal.a(i)*fi*alpha/vi;

                break;
            }
        }
    }

    return tTotalConcentration;
}


Foam::tmp<Foam::volScalarField>
Foam::functionObjects::populationBalanceMoments::mean
(
    const populationBalanceModel& popBal
)
{
    tmp<volScalarField> tMean
    (
        volScalarField::New
        (
            "mean",
            mesh_,
            dimensionedScalar(dimless, Zero)
        )
    );

    volScalarField& mean = tMean.ref();

    setDimensions(mean, momentType::mean);

    volScalarField totalConcentration(this->totalConcentration(popBal));

    forAll(popBal.fs(), i)
    {
        const volScalarField& alpha = popBal.phases()[i];
        const volScalarField& fi = popBal.f(i);
        const dimensionedScalar& vi = popBal.v(i);

        volScalarField concentration(fi*alpha/vi);

        switch (weightType_)
        {
            case weightType::volumeConcentration:
            {
                concentration *= vi;

                break;
            }
            case weightType::areaConcentration:
            {
                concentration *= popBal.a(i);

                break;
            }
            default:
            {
                break;
            }
        }

        switch (meanType_)
        {
            case meanType::geometric:
            {
                mean.dimensions().reset(dimless);

                switch (coordinateType_)
                {
                    case coordinateType::volume:
                    {
                        dimensionedScalar unitVolume(dimVolume, 1);

                        mean +=
                            Foam::log(vi/unitVolume)
                           *concentration/totalConcentration;

                        break;
                    }
                    case coordinateType::area:
                    {
                        dimensionedScalar unitArea(dimArea, 1);

                        mean +=
                            Foam::log(popBal.a(i)/unitArea)
                           *concentration/totalConcentration;

                        break;
                    }
                    case coordinateType::diameter:
                    {
                        dimensionedScalar unitLength(dimLength, 1);

                        mean +=
                            Foam::log(popBal.d(i)/unitLength)
                           *concentration/totalConcentration;

                        break;
                    }
                }

                break;
            }
            default:
            {
                switch (coordinateType_)
                {
                    case coordinateType::volume:
                    {
                        mean += vi*concentration/totalConcentration;

                        break;
                    }
                    case coordinateType::area:
                    {
                        mean += popBal.a(i)*concentration/totalConcentration;

                        break;
                    }
                    case coordinateType::diameter:
                    {
                        mean += popBal.d(i)*concentration/totalConcentration;

                        break;
                    }
                }

                break;
            }
        }
    }

    if (meanType_ == meanType::geometric)
    {
        mean = exp(mean);

        setDimensions(mean, momentType::mean);
    }

    return tMean;
}


Foam::tmp<Foam::volScalarField>
Foam::functionObjects::populationBalanceMoments::variance
(
    const populationBalanceModel& popBal
)
{
    tmp<volScalarField> tVariance
    (
        volScalarField::New
        (
            "variance",
            mesh_,
            dimensionedScalar(dimless, Zero)
        )
    );

    volScalarField& variance = tVariance.ref();

    setDimensions(variance, momentType::variance);

    volScalarField totalConcentration(this->totalConcentration(popBal));
    volScalarField mean(this->mean(popBal));

    forAll(popBal.fs(), i)
    {
        const volScalarField& alpha = popBal.phases()[i];
        const volScalarField& fi = popBal.f(i);
        const dimensionedScalar& vi = popBal.v(i);

        volScalarField concentration(fi*alpha/vi);

        switch (weightType_)
        {
            case weightType::volumeConcentration:
            {
                concentration *= vi;

                break;
            }
            case weightType::areaConcentration:
            {
                concentration *= popBal.a(i);

                break;
            }
            default:
            {
                break;
            }
        }

        switch (meanType_)
        {
            case meanType::geometric:
            {
                switch (coordinateType_)
                {
                    case coordinateType::volume:
                    {
                        variance +=
                            sqr(Foam::log(vi/mean))
                           *concentration/totalConcentration;

                        break;
                    }
                    case coordinateType::area:
                    {
                        variance +=
                            sqr(Foam::log(popBal.a(i)/mean))
                           *concentration/totalConcentration;

                        break;
                    }
                    case coordinateType::diameter:
                    {
                        variance +=
                            sqr(Foam::log(popBal.d(i)/mean))
                           *concentration/totalConcentration;

                        break;
                    }
                }

                break;
            }
            default:
            {
                switch (coordinateType_)
                {
                    case coordinateType::volume:
                    {
                        variance +=
                            sqr(vi - mean)*concentration/totalConcentration;

                        break;
                    }
                    case coordinateType::area:
                    {
                        variance +=
                            sqr(popBal.a(i) - mean)
                           *concentration
                           /totalConcentration;

                        break;
                    }
                    case coordinateType::diameter:
                    {
                        variance +=
                            sqr(popBal.d(i) - mean)
                           *concentration
                           /totalConcentration;

                        break;
                    }
                }

                break;
            }
        }
    }

    return tVariance;
}


Foam::tmp<Foam::volScalarField>
Foam::functionObjects::populationBalanceMoments::stdDev
(
    const populationBalanceModel& popBal
)
{
    switch (meanType_)
    {
        case meanType::geometric:
        {
            return exp(sqrt(this->variance(popBal)));
        }
        default:
        {
            return sqrt(this->variance(popBal));
        }
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::populationBalanceMoments::populationBalanceMoments
(
    const word& name,
    const Time& runTime,
    const dictionary& dict
)
:
    fvMeshFunctionObject(name, runTime, dict),
    popBalName_(dict.lookup("populationBalance")),
    momentType_(momentTypeNames_.read(dict.lookup("momentType"))),
    coordinateType_(coordinateTypeNames_.read(dict.lookup("coordinateType"))),
    weightType_
    (
        dict.found("weightType")
      ? weightTypeNames_.read(dict.lookup("weightType"))
      : weightType::numberConcentration
    ),
    meanType_(meanType::notApplicable),
    order_(-1),
    fldPtr_(nullptr)
{
    read(dict);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::functionObjects::populationBalanceMoments::~populationBalanceMoments()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool
Foam::functionObjects::populationBalanceMoments::read(const dictionary& dict)
{
    fvMeshFunctionObject::read(dict);

    switch (momentType_)
    {
        case momentType::integerMoment:
        {
            order_ = dict.lookup<int>("order");

            break;
        }
        default:
        {
            meanType_ =
                dict.found("meanType")
              ? meanTypeNames_.read(dict.lookup("meanType"))
              : meanType::arithmetic;

            break;
        }
    }

    switch (momentType_)
    {
        case momentType::integerMoment:
        {
            fldPtr_.set
            (
                new volScalarField
                (
                    IOobject
                    (
                        this->integerMomentFldName(),
                        mesh_.time().name(),
                        mesh_,
                        IOobject::NO_READ,
                        IOobject::NO_WRITE
                    ),
                    mesh_,
                    dimensionedScalar(dimless, Zero)
                )
            );

            volScalarField& integerMoment = fldPtr_();

            setDimensions(integerMoment, momentType::integerMoment);

            break;
        }
        case momentType::mean:
        {
            fldPtr_.set
            (
                new volScalarField
                (
                    IOobject
                    (
                        this->defaultFldName(),
                        mesh_.time().name(),
                        mesh_,
                        IOobject::NO_READ,
                        IOobject::NO_WRITE
                    ),
                    mesh_,
                    dimensionedScalar(dimless, Zero)
                )
            );

            setDimensions(fldPtr_(), momentType::mean);

            break;
        }
        case momentType::variance:
        {
            fldPtr_.set
            (
                new volScalarField
                (
                    IOobject
                    (
                        this->defaultFldName(),
                        mesh_.time().name(),
                        mesh_,
                        IOobject::NO_READ,
                        IOobject::NO_WRITE
                    ),
                    mesh_,
                    dimensionedScalar(dimless, Zero)
                )
            );

            setDimensions(fldPtr_(), momentType::variance);

            break;
        }
        case momentType::stdDev:
        {
            fldPtr_.set
            (
                new volScalarField
                (
                    IOobject
                    (
                        this->defaultFldName(),
                        mesh_.time().name(),
                        mesh_,
                        IOobject::NO_READ,
                        IOobject::NO_WRITE
                    ),
                    mesh_,
                    dimensionedScalar(dimless, Zero)
                )
            );

            setDimensions(fldPtr_(), momentType::stdDev);

            break;
        }
    }

    return true;
}


bool Foam::functionObjects::populationBalanceMoments::execute()
{
    const populationBalanceModel& popBal =
        obr_.lookupObject<populationBalanceModel>(popBalName_);

    switch (momentType_)
    {
        case momentType::integerMoment:
        {
            volScalarField& integerMoment = fldPtr_();

            integerMoment = Zero;

            forAll(popBal.fs(), i)
            {
                const volScalarField& alpha = popBal.phases()[i];
                const volScalarField& fi = popBal.f(i);
                const dimensionedScalar& vi = popBal.v(i);

                volScalarField concentration(fi*alpha/vi);

                switch (weightType_)
                {
                    case weightType::volumeConcentration:
                    {
                        concentration *= vi;

                        break;
                    }
                    case weightType::areaConcentration:
                    {
                        concentration *= popBal.a(i);

                        break;
                    }
                    default:
                    {
                        break;
                    }
                }

                switch (coordinateType_)
                {
                    case coordinateType::volume:
                    {
                        integerMoment +=
                            pow(vi, order_)*concentration;

                        break;
                    }
                    case coordinateType::area:
                    {
                        integerMoment +=
                            pow(popBal.a(i), order_)*concentration;

                        break;
                    }
                    case coordinateType::diameter:
                    {
                        integerMoment +=
                            pow(popBal.d(i), order_)*concentration;

                        break;
                    }
                }
            }

            break;
        }
        case momentType::mean:
        {
            fldPtr_() = this->mean(popBal);

            break;
        }
        case momentType::variance:
        {
            fldPtr_() = this->variance(popBal);

            break;
        }
        case momentType::stdDev:
        {
            fldPtr_() = sqrt(this->variance(popBal));

            break;
        }
    }

    return true;
}


bool Foam::functionObjects::populationBalanceMoments::write()
{
    writeObject(fldPtr_->name());

    return true;
}


// ************************************************************************* //
