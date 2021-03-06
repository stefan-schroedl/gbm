//  GBM by Greg Ridgeway  Copyright (C) 2003

#include "quantile.h"


void CQuantile::ComputeWorkingResponse
(
    const double *adY,
    const double *adMisc,
    const double *adOffset,
    const double *adF,
    double *adZ,
    const double *adWeight,
    const bag& afInBag,
    unsigned long nTrain,
    int cIdxOff
)
{
    unsigned long i = 0;

    if(adOffset == NULL)
    {
        for(i=0; i<nTrain; i++)
        {
            adZ[i] = (adY[i] > adF[i]) ? dAlpha : -(1.0-dAlpha);
        }
    }
    else
    {
        for(i=0; i<nTrain; i++)
        {
            adZ[i] = (adY[i] > adF[i]+adOffset[i]) ? dAlpha : -(1.0-dAlpha);
        }
    }
}


void CQuantile::InitF
(
    const double *adY,
    const double *adMisc,
    const double *adOffset,
    const double *adWeight,
    double &dInitF,
    unsigned long cLength
)
{
    double dOffset=0.0;
    unsigned long i=0;
    int nLength = int(cLength);

    vecd.resize(cLength);
    for(i=0; i<cLength; i++)
    {
        dOffset = (adOffset==NULL) ? 0.0 : adOffset[i];
        vecd[i] = adY[i] - dOffset;
    }

    dInitF = mpLocM.weightedQuantile(nLength, &vecd[0], adWeight, dAlpha);
}


double CQuantile::Deviance
(
    const double *adY,
    const double *adMisc,
    const double *adOffset,
    const double *adWeight,
    const double *adF,
    unsigned long cLength,
    int cIdxOff
)
{
    unsigned long i=0;
    double dL = 0.0;
    double dW = 0.0;

    if(adOffset == NULL)
    {
        for(i=cIdxOff; i<cLength+cIdxOff; i++)
        {
            if(adY[i] > adF[i])
            {
                dL += adWeight[i]*dAlpha      *(adY[i] - adF[i]);
            }
            else
            {
                dL += adWeight[i]*(1.0-dAlpha)*(adF[i] - adY[i]);
            }
            dW += adWeight[i];
        }
    }
    else
    {
        for(i=cIdxOff; i<cLength+cIdxOff; i++)
        {
            if(adY[i] > adF[i] + adOffset[i])
            {
                dL += adWeight[i]*dAlpha      *(adY[i] - adF[i]-adOffset[i]);
            }
            else
            {
                dL += adWeight[i]*(1.0-dAlpha)*(adF[i]+adOffset[i] - adY[i]);
            }
            dW += adWeight[i];
        }
    }

    return dL/dW;
}

void CQuantile::FitBestConstant
(
    const double *adY,
    const double *adMisc,
    const double *adOffset,
    const double *adW,
    const double *adF,
    double *adZ,
    const std::vector<unsigned long> &aiNodeAssign,
    unsigned long nTrain,
    VEC_P_NODETERMINAL vecpTermNodes,
    unsigned long cTermNodes,
    unsigned long cMinObsInNode,
    const bag& afInBag,
    const double *adFadj,
	int cIdxOff
)
{
  unsigned long iNode = 0;
  unsigned long iObs = 0;
  unsigned long iVecd = 0;
  double dOffset;

  vecd.resize(nTrain); // should already be this size from InitF
  std::vector<double> adW2(nTrain);

  for(iNode=0; iNode<cTermNodes; iNode++)
    {
      if(vecpTermNodes[iNode]->cN >= cMinObsInNode)
        {
	  iVecd = 0;
	  for(iObs=0; iObs<nTrain; iObs++)
            {
	      if(afInBag[iObs] && (aiNodeAssign[iObs] == iNode))
                {
		  dOffset = (adOffset==NULL) ? 0.0 : adOffset[iObs];
		  
		  vecd[iVecd] = adY[iObs] - dOffset - adF[iObs];
		  adW2[iVecd] = adW[iObs];
		  iVecd++;
                }
            }
	  
	  vecpTermNodes[iNode]->dPrediction = mpLocM.weightedQuantile(iVecd, &vecd[0], &adW2[0], dAlpha);
	}
    }
}



double CQuantile::BagImprovement
(
    const double *adY,
    const double *adMisc,
    const double *adOffset,
    const double *adWeight,
    const double *adF,
    const double *adFadj,
    const bag& afInBag,
    double dStepSize,
    unsigned long nTrain
)
{
    double dReturnValue = 0.0;

    double dF = 0.0;
    double dW = 0.0;
    unsigned long i = 0;

    for(i=0; i<nTrain; i++)
    {
        if(!afInBag[i])
        {
            dF = adF[i] + ((adOffset==NULL) ? 0.0 : adOffset[i]);
            if(adY[i] > dF)
            {
                dReturnValue += adWeight[i]*dAlpha*(adY[i]-dF);
            }
            else
            {
                dReturnValue += adWeight[i]*(1-dAlpha)*(dF-adY[i]);
            }

            if(adY[i] > dF+dStepSize*adFadj[i])
            {
                dReturnValue -= adWeight[i]*dAlpha*
                                (adY[i] - dF-dStepSize*adFadj[i]);
            }
            else
            {
                dReturnValue -= adWeight[i]*(1-dAlpha)*
                                (dF+dStepSize*adFadj[i] - adY[i]);
            }
            dW += adWeight[i];
        }
    }

    return dReturnValue/dW;
}

