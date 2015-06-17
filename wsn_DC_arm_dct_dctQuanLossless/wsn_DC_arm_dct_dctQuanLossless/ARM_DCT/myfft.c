#include "arm_math.h"
#include <math.h>
#include <stdlib.h>

//flag = false Îªfft£¬trueÎªifft
void MYFFT(double *Re, double *Im, int n, int flag)
{
	int k,it=0;
	int i,time,index;
	int baseIndex;
	int sign;
	double *wr, *wi;
	int step;
	double dataR,dataI;
	int index1,index2;
	int pot=0;
	int *line;
	double *dr;
	double *di;
	int wValue;
	int stepW;
	int muli;

	for(k = 1; ; k *= 2)	
	{
		if(k == n) break;
		it++;
		if(it > 64) return;
	}
	//int *line = new int[n];
	//double *dr = new double[n];
	//double *di = new double[n];

	line = (int *)malloc(n * sizeof(int));
	dr = (double *)malloc(n * sizeof(double));
	di = (double *)malloc(n * sizeof(double));


	line[0] = 0;
	for (time=0;time<it;time++)
	{
		index = (int)pow(2.0f,time);
		for (i=0;i<index;i++)
		{	
			line[index+i] = line[i] + (int)pow(2.0f,it-1-time);
		}
	}


	if(flag)
		sign = 1;
	else
		sign = -1;

	for (i=0;i<n;i++)
	{
		dr[i] = Re[(int)line[i]];
		di[i] =Im[(int)line[i]];
	}


	//double *wr = new double[n/2];
	//double *wi = new double[n/2];

	wr = (double *)malloc(n/2 * sizeof(double));
	wi = (double *)malloc(n/2 * sizeof(double));

	for (i=0;i<n/2;i++)
	{
		wr[i] = cos( i*2*PI/n );
		wi[i] = sign*sin(i*2*PI/n);
	}

	for (step=1;step<=it;step++)
	{
		for (baseIndex=0;baseIndex<n;baseIndex+=(int)pow(2.0f,step))
		{
			
			stepW = ( (int)pow(2.0f,it-step) )%(n/2);
			for (muli=0;muli<pow(2.0f,step-1);muli++)
			{

				wValue = muli*stepW;

				index1 = baseIndex+muli;
				index2 = baseIndex+(int)pow(2.0f,step-1)+muli;

				dataR = dr[index2]*wr[wValue] - di[index2]*wi[wValue];
				dataI = dr[index2]*wi[wValue] + di[index2]*wr[wValue];

				dr[index2] = dr[index1]-dataR;
				di[index2] = di[index1]-dataI;
				dr[index1] += dataR;
				di[index1] += dataI;
			}
		}
	}

	for (i=0;i<n;i++)
	{
		Re[i] = dr[i];
		Im[i] = di[i];
	}
	free(dr);
	free(di);
	free(wr);
	free(wi);
	free(line);
}


void my_rfft_f32(float32_t * pSrc, float32_t * pDst, uint16_t N, int flag)
{
	double *Re, *Im;
	double *pD1, *pD2;
	float32_t *pS1;
	uint16_t i;
	pS1 = pSrc;
	Re = (double *)malloc(N * sizeof(double));
	Im = (double *)malloc(N * sizeof(double));

	pD1 = Re;
	pD2 = Im;
	i = N;
	do
	{
		*pD1++ = (double)(*pS1++);
		*pD2++ = 0.0f;
		i--;
	}while(i > 0u);


	MYFFT(Re, Im, N, flag);

	pS1 = pDst;
	pD1 = Re;
	pD2 = Im;
	i = N * 2;
	do
	{
		*pS1++ = (float)(*pD1++);
		*pS1++ = (float)(*pD2++);

		i--;
	}while(i > 0u);
}
