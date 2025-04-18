/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2021-2024 OpenFOAM Foundation
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

#include "MaxwellStefan.H"
#include "fvcDiv.H"
#include "fvcLaplacian.H"
#include "fvcSnGrad.H"
#include "fvmSup.H"
#include "surfaceInterpolate.H"
#include "Function2Evaluate.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

template<class BasicThermophysicalTransportModel>
void MaxwellStefan<BasicThermophysicalTransportModel>::
transformDiffusionCoefficient() const
{
    const label d = this->thermo().defaultSpecie();

    // Calculate the molecular weight of the mixture and the mole fractions
    scalar Wm = 0;

    forAll(W, i)
    {
        X[i] = Y[i]/W[i];
        Wm += X[i];
    }

    Wm = 1/Wm;
    X *= Wm;

    // i counter for the specie sub-system without the default specie
    label is = 0;

    // Calculate the A and B matrices from the binary mass diffusion
    // coefficients and specie mole fractions
    forAll(X, i)
    {
        if (i != d)
        {
            A(is, is) = -X[i]*Wm/(DD(i, d)*W[d]);
            B(is, is) = -(X[i]*Wm/W[d] + (1 - X[i])*Wm/W[i]);

            // j counter for the specie sub-system without the default specie
            label js = 0;

            forAll(X, j)
            {
                if (j != i)
                {
                    A(is, is) -= X[j]*Wm/(DD(i, j)*W[i]);

                    if (j != d)
                    {
                        A(is, js) =
                            X[i]*(Wm/(DD(i, j)*W[j]) - Wm/(DD(i, d)*W[d]));

                        B(is, js) = X[i]*(Wm/W[j] - Wm/W[d]);
                    }
                }

                if (j != d)
                {
                    js++;
                }
            }

            is++;
        }
    }

    // LU decompose A and invert
    A.decompose();
    A.inv(invA);

    // Calculate the generalised Fick's law diffusion coefficients
    multiply(D, invA, B);
}


template<class BasicThermophysicalTransportModel>
void MaxwellStefan<BasicThermophysicalTransportModel>::
transformDiffusionCoefficientFields() const
{
    const label d = this->thermo().defaultSpecie();

    // For each cell or patch face
    forAll(*(YPtrs[0]), pi)
    {
        forAll(W, i)
        {
            // Map YPtrs -> Y
            Y[i] = (*YPtrs[i])[pi];

            // Map DijPtrs -> DD
            forAll(W, j)
            {
                DD(i, j) = (*DijPtrs[i][j])[pi];
            }
        }

        // Transform DD -> D
        transformDiffusionCoefficient();

        // i counter for the specie sub-system without the default specie
        label is = 0;

        forAll(W, i)
        {
            if (i != d)
            {
                // j counter for the specie sub-system
                // without the default specie
                label js = 0;

                // Map D -> DijPtrs
                forAll(W, j)
                {
                    if (j != d)
                    {
                        (*DijPtrs[i][j])[pi] = D(is, js);

                        js++;
                    }
                }

                is++;
            }
        }
    }
}


template<class BasicThermophysicalTransportModel>
void MaxwellStefan<BasicThermophysicalTransportModel>::transform
(
    List<PtrList<volScalarField>>& Dij
) const
{
    const PtrList<volScalarField>& Y = this->thermo().Y();
    const volScalarField& Y0 = Y[0];

    forAll(W, i)
    {
        // Map this->thermo().Y() internal fields -> YPtrs
        YPtrs[i] = &Y[i].primitiveField();

        // Map Dii_ internal fields -> DijPtrs
        DijPtrs[i][i] = &Dii_[i].primitiveFieldRef();

        // Map Dij internal fields -> DijPtrs
        forAll(W, j)
        {
            if (j != i)
            {
                DijPtrs[i][j] = &Dij[i][j].primitiveFieldRef();
            }
        }
    }

    // Transform binary mass diffusion coefficients internal field DijPtrs ->
    // generalised Fick's law diffusion coefficients DijPtrs
    transformDiffusionCoefficientFields();

    forAll(Y0.boundaryField(), patchi)
    {
        forAll(W, i)
        {
            // Map this->thermo().Y() patch fields -> YPtrs
            YPtrs[i] = &Y[i].boundaryField()[patchi];

            // Map Dii_ patch fields -> DijPtrs
            DijPtrs[i][i] = &Dii_[i].boundaryFieldRef()[patchi];

            // Map Dij patch fields -> DijPtrs
            forAll(W, j)
            {
                if (j != i)
                {
                    DijPtrs[i][j] = &Dij[i][j].boundaryFieldRef()[patchi];
                }
            }
        }

        // Transform binary mass diffusion coefficients patch field DijPtrs ->
        // generalised Fick's law diffusion coefficients DijPtrs
        transformDiffusionCoefficientFields();
    }
}


template<class BasicThermophysicalTransportModel>
void MaxwellStefan<BasicThermophysicalTransportModel>::updateDii() const
{
    const label d = this->thermo().defaultSpecie();

    const PtrList<volScalarField>& Y = this->thermo().Y();
    const volScalarField& p = this->thermo().p();
    const volScalarField& T = this->thermo().T();
    const volScalarField& rho = this->momentumTransport().rho();

    Dii_.setSize(Y.size());
    jexp_.setSize(Y.size());

    List<PtrList<volScalarField>> Dij(Y.size());

    // Evaluate the specie binary mass diffusion coefficient functions
    // and initialise the explicit part of the specie mass flux fields
    forAll(Y, i)
    {
        if (i != d)
        {
            if (jexp_.set(i))
            {
                jexp_[i] = Zero;
            }
            else
            {
                jexp_.set
                (
                    i,
                    surfaceScalarField::New
                    (
                        "jexp" + Y[i].name(),
                        Y[i].mesh(),
                        dimensionedScalar(dimensionSet(1, -2, -1, 0, 0), 0)
                    )
                );
            }
        }

        Dii_.set(i, evaluate(DFuncs_[i][i], dimKinematicViscosity, p, T));

        Dij[i].setSize(Y.size());

        forAll(Y, j)
        {
            if (j > i)
            {
                Dij[i].set
                (
                    j,
                    evaluate(DFuncs_[i][j], dimKinematicViscosity, p, T)
                );
            }
            else if (j < i)
            {
                Dij[i].set(j, Dij[j][i].clone());
            }
        }
    }

    //- Transform the binary mass diffusion coefficients into the
    //  the generalised Fick's law diffusion coefficients
    transform(Dij);

    // Accumulate the explicit part of the specie mass flux fields
    forAll(Y, j)
    {
        if (j != d)
        {
            const surfaceScalarField snGradYj(fvc::snGrad(Y[j]));

            forAll(Y, i)
            {
                if (i != d && i != j)
                {
                    jexp_[i] -= fvc::interpolate(rho*Dij[i][j])*snGradYj;
                }
            }
        }
    }

    // Optionally add the Soret thermal diffusion contribution to the
    // explicit part of the specie mass flux fields
    if (DTFuncs_.size())
    {
        const surfaceScalarField gradTbyT(fvc::snGrad(T)/fvc::interpolate(T));

        forAll(Y, i)
        {
            if (i != d)
            {
                jexp_[i] -= fvc::interpolate
                (
                    evaluate(DTFuncs_[i], dimDynamicViscosity, p, T)
                )*gradTbyT;
            }
        }
    }
}


template<class BasicThermophysicalTransportModel>
const PtrList<volScalarField>&
MaxwellStefan<BasicThermophysicalTransportModel>::Dii() const
{
    if (!Dii_.size())
    {
        updateDii();
    }

    return Dii_;
}


template<class BasicThermophysicalTransportModel>
const PtrList<surfaceScalarField>&
MaxwellStefan<BasicThermophysicalTransportModel>::jexp() const
{
    if (!jexp_.size())
    {
        updateDii();
    }

    return jexp_;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class BasicThermophysicalTransportModel>
MaxwellStefan<BasicThermophysicalTransportModel>::MaxwellStefan
(
    const word& type,
    const momentumTransportModel& momentumTransport,
    const thermoModel& thermo
)
:
    BasicThermophysicalTransportModel
    (
        type,
        momentumTransport,
        thermo
    ),

    TopoChangeableMeshObject(*this),

    DFuncs_(this->thermo().species().size()),

    DTFuncs_
    (
        this->coeffDict().found("DT")
      ? this->thermo().species().size()
      : 0
    ),

    W(this->thermo().species().size()),

    YPtrs(W.size()),
    DijPtrs(W.size()),

    Y(W.size()),
    X(W.size()),
    DD(W.size()),
    A(W.size() - 1),
    B(A.m()),
    invA(A.m()),
    D(W.size())
{
    // Set the molecular weights of the species
    forAll(W, i)
    {
        W[i] = this->thermo().Wi(i).value();
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class BasicThermophysicalTransportModel>
bool MaxwellStefan<BasicThermophysicalTransportModel>::read()
{
    if
    (
        BasicThermophysicalTransportModel::read()
    )
    {
        const speciesTable& species = this->thermo().species();

        const dictionary& coeffDict = this->coeffDict();
        const dictionary& Ddict = coeffDict.subDict("D");

        // Read the array of specie binary mass diffusion coefficient functions
        forAll(species, i)
        {
            DFuncs_[i].setSize(species.size());

            forAll(species, j)
            {
                if (j >= i)
                {
                    const word nameij(species[i] + '-' + species[j]);
                    const word nameji(species[j] + '-' + species[i]);

                    word Dname;

                    if (Ddict.found(nameij) && Ddict.found(nameji))
                    {
                        if (i != j)
                        {
                            WarningInFunction
                                << "Binary mass diffusion coefficients "
                                   "for both " << nameij << " and " << nameji
                                << " provided, using " << nameij << endl;
                        }

                        Dname = nameij;
                    }
                    else if (Ddict.found(nameij))
                    {
                        Dname = nameij;
                    }
                    else if (Ddict.found(nameji))
                    {
                        Dname = nameji;
                    }
                    else
                    {
                        FatalIOErrorInFunction(Ddict)
                            << "Binary mass diffusion coefficients for pair "
                            << nameij << " or " << nameji << " not provided"
                            << exit(FatalIOError);
                    }

                    DFuncs_[i].set
                    (
                        j,
                        Function2<scalar>::New
                        (
                            Dname,
                            dimPressure,
                            dimTemperature,
                            dimKinematicViscosity,
                            Ddict
                        ).ptr()
                    );
                }
            }
        }

        // Optionally read the List of specie Soret thermal diffusion
        // coefficient functions
        if (coeffDict.found("DT"))
        {
            const dictionary& DTdict = coeffDict.subDict("DT");

            forAll(species, i)
            {
                DTFuncs_.set
                (
                    i,
                    Function2<scalar>::New
                    (
                        species[i],
                        dimPressure,
                        dimTemperature,
                        dimDynamicViscosity,
                        DTdict
                    ).ptr()
                );
            }
        }

        return true;
    }
    else
    {
        return false;
    }
}


template<class BasicThermophysicalTransportModel>
tmp<volScalarField> MaxwellStefan<BasicThermophysicalTransportModel>::DEff
(
    const volScalarField& Yi
) const
{
    return volScalarField::New
    (
        "DEff",
        this->momentumTransport().rho()*Dii()[this->thermo().specieIndex(Yi)]
    );
}


template<class BasicThermophysicalTransportModel>
tmp<scalarField> MaxwellStefan<BasicThermophysicalTransportModel>::DEff
(
    const volScalarField& Yi,
    const label patchi
) const
{
    return
        this->momentumTransport().rho().boundaryField()[patchi]
       *Dii()[this->thermo().specieIndex(Yi)].boundaryField()[patchi];
}


template<class BasicThermophysicalTransportModel>
tmp<surfaceScalarField>
MaxwellStefan<BasicThermophysicalTransportModel>::q() const
{
    tmp<surfaceScalarField> tmpq
    (
        surfaceScalarField::New
        (
            IOobject::groupName
            (
                "q",
                this->momentumTransport().alphaRhoPhi().group()
            ),
           -fvc::interpolate(this->alpha()*this->kappaEff())
           *fvc::snGrad(this->thermo().T())
        )
    );

    const label d = this->thermo().defaultSpecie();

    const PtrList<volScalarField>& Y = this->thermo().Y();
    const volScalarField& p = this->thermo().p();
    const volScalarField& T = this->thermo().T();

    if (Y.size())
    {
        surfaceScalarField sumJ
        (
            surfaceScalarField::New
            (
                "sumJ",
                Y[0].mesh(),
                dimensionedScalar(dimMass/dimArea/dimTime, 0)
            )
        );

        surfaceScalarField sumJh
        (
            surfaceScalarField::New
            (
                "sumJh",
                Y[0].mesh(),
                dimensionedScalar(sumJ.dimensions()*dimEnergy/dimMass, 0)
            )
        );

        forAll(Y, i)
        {
            if (i != d)
            {
                const volScalarField hi(this->thermo().hsi(i, p, T));

                const surfaceScalarField ji(this->j(Y[i]));

                sumJ += ji;

                sumJh += ji*fvc::interpolate(hi);
            }
        }

        {
            const label i = d;

            const volScalarField hi(this->thermo().hsi(i, p, T));

            sumJh -= sumJ*fvc::interpolate(hi);
        }

        tmpq.ref() += sumJh;
    }

    return tmpq;
}


template<class BasicThermophysicalTransportModel>
tmp<scalarField> MaxwellStefan<BasicThermophysicalTransportModel>::q
(
    const label patchi
) const
{
    tmp<scalarField> tmpq
    (
      - (
            this->alpha().boundaryField()[patchi]
           *this->kappaEff(patchi)
           *this->thermo().T().boundaryField()[patchi]
        )
    );

    const label d = this->thermo().defaultSpecie();

    const PtrList<volScalarField>& Y = this->thermo().Y();
    const volScalarField& p = this->thermo().p();
    const volScalarField& T = this->thermo().T();

    if (Y.size())
    {
        scalarField sumJ(tmpq->size(), scalar(0));
        scalarField sumJh(tmpq->size(), scalar(0));

        forAll(Y, i)
        {
            if (i != d)
            {
                const scalarField hi(this->thermo().hsi(i, p, T));

                const scalarField ji(this->j(Y[i], patchi));

                sumJ += ji;

                sumJh += ji*hi;
            }
        }

        {
            const label i = d;

            const scalarField hi(this->thermo().hsi(i, p, T));

            sumJh -= sumJ*hi;
        }

        tmpq.ref() += sumJh;
    }

    return tmpq;
}


template<class BasicThermophysicalTransportModel>
tmp<fvScalarMatrix> MaxwellStefan<BasicThermophysicalTransportModel>::divq
(
    volScalarField& he
) const
{
    tmp<fvScalarMatrix> tmpDivq
    (
        fvm::Su
        (
            -fvc::laplacian(this->alpha()*this->kappaEff(), this->thermo().T()),
            he
        )
    );

    const label d = this->thermo().defaultSpecie();

    const PtrList<volScalarField>& Y = this->thermo().Y();
    const volScalarField& p = this->thermo().p();
    const volScalarField& T = this->thermo().T();

    tmpDivq.ref() -=
        fvm::laplacianCorrection(this->alpha()*this->alphaEff(), he);

    surfaceScalarField sumJ
    (
        surfaceScalarField::New
        (
            "sumJ",
            he.mesh(),
            dimensionedScalar(dimMass/dimArea/dimTime, 0)
        )
    );

    surfaceScalarField sumJh
    (
        surfaceScalarField::New
        (
            "sumJh",
            he.mesh(),
            dimensionedScalar(sumJ.dimensions()*he.dimensions(), 0)
        )
    );

    forAll(Y, i)
    {
        if (i != d)
        {
            const volScalarField hi(this->thermo().hsi(i, p, T));

            const surfaceScalarField ji(this->j(Y[i]));

            sumJ += ji;

            sumJh += ji*fvc::interpolate(hi);
        }
    }

    {
        const label i = d;

        const volScalarField hi(this->thermo().hsi(i, p, T));

        sumJh -= sumJ*fvc::interpolate(hi);
    }

    tmpDivq.ref() += fvc::div(sumJh*he.mesh().magSf());

    return tmpDivq;
}


template<class BasicThermophysicalTransportModel>
tmp<surfaceScalarField> MaxwellStefan<BasicThermophysicalTransportModel>::j
(
    const volScalarField& Yi
) const
{
    const label d = this->thermo().defaultSpecie();

    if (this->thermo().specieIndex(Yi) == d)
    {
        const PtrList<volScalarField>& Y = this->thermo().Y();

        tmp<surfaceScalarField> tjd
        (
            surfaceScalarField::New
            (
                IOobject::groupName
                (
                    "j" + name(d),
                    this->momentumTransport().alphaRhoPhi().group()
                ),
                Yi.mesh(),
                dimensionedScalar(dimMass/dimArea/dimTime, 0)
            )
        );

        surfaceScalarField& jd = tjd.ref();

        forAll(Y, i)
        {
            if (i != d)
            {
                jd -= this->j(Y[i]);
            }
        }

        return tjd;
    }
    else
    {
        return
            BasicThermophysicalTransportModel::j(Yi)
          + jexp()[this->thermo().specieIndex(Yi)];
    }
}


template<class BasicThermophysicalTransportModel>
tmp<scalarField> MaxwellStefan<BasicThermophysicalTransportModel>::j
(
    const volScalarField& Yi,
    const label patchi
) const
{
    const label d = this->thermo().defaultSpecie();

    if (this->thermo().specieIndex(Yi) == d)
    {
        const PtrList<volScalarField>& Y = this->thermo().Y();

        tmp<scalarField> tjd
        (
            new scalarField(Yi.boundaryField()[patchi].size(), scalar(0))
        );
        scalarField& jd = tjd.ref();

        forAll(Y, i)
        {
            if (i != d)
            {
                jd -= this->j(Y[i], patchi);
            }
        }

        return tjd;
    }
    else
    {
        return
            BasicThermophysicalTransportModel::j(Yi, patchi)
          + jexp()[this->thermo().specieIndex(Yi)].boundaryField()[patchi];
    }
}


template<class BasicThermophysicalTransportModel>
tmp<fvScalarMatrix> MaxwellStefan<BasicThermophysicalTransportModel>::divj
(
    volScalarField& Yi
) const
{
    return
        BasicThermophysicalTransportModel::divj(Yi)
      + fvc::div(jexp()[this->thermo().specieIndex(Yi)]*Yi.mesh().magSf());
}


template<class BasicThermophysicalTransportModel>
void MaxwellStefan<BasicThermophysicalTransportModel>::predict()
{
    BasicThermophysicalTransportModel::predict();
    updateDii();
}


template<class BasicThermophysicalTransportModel>
bool MaxwellStefan<BasicThermophysicalTransportModel>::movePoints()
{
    return true;
}


template<class BasicThermophysicalTransportModel>
void MaxwellStefan<BasicThermophysicalTransportModel>::topoChange
(
    const polyTopoChangeMap& map
)
{
    // Delete the cached Dii and jexp, will be re-created in predict
    Dii_.clear();
    jexp_.clear();
}


template<class BasicThermophysicalTransportModel>
void MaxwellStefan<BasicThermophysicalTransportModel>::mapMesh
(
    const polyMeshMap& map
)
{
    // Delete the cached Dii and jexp, will be re-created in predict
    Dii_.clear();
    jexp_.clear();
}


template<class BasicThermophysicalTransportModel>
void MaxwellStefan<BasicThermophysicalTransportModel>::distribute
(
    const polyDistributionMap& map
)
{
    // Delete the cached Dii and jexp, will be re-created in predict
    Dii_.clear();
    jexp_.clear();
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
