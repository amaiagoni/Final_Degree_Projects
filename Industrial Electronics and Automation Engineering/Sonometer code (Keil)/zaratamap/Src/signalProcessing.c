#include "signalProcessing.h"
#include "arm_math.h"


	#define NUM_TAPS              15
	#define NUM_STAGES						3
	#define FILTER_NUMBER					11
	
	float inDataFloat[FRAME_LENGTH];
	float valueRMSTemp[FILTER_NUMBER + 1];
	float outDataFilter[FRAME_LENGTH];
	int 	frameNumber = 0;
	
	/* 	Variables to reuse memory for the different arm_biquad_cascade filters	 */
	
  float32_t *pIn;                        		 		 /*  source pointer            */
  float32_t *pOut;                        			 /*  destination pointer       */
  float32_t *pState;                 						 /*  pState pointer            */
  float32_t *pCoeffs;               						 /*  coefficient pointer       */
  float32_t acc;                          			 /*  Simulates the accumulator */
  float32_t b0, b1, b2, a1, a2;           			 /*  Filter coefficients       */
  float32_t Xn1, Xn2, Yn1, Yn2;          				 /*  Filter pState variables   */
  float32_t Xn;                           			 /*  temporary input           */
  uint32_t sample, stage;         							 /*  loop counters             */
	
	static float32_t filterCoefficients[FILTER_NUMBER][NUM_TAPS];

	const float32_t coefficientsOctave31_5Hz[NUM_TAPS] = {
		0.0014512,-3.7657e-11,-0.0014512,1.9982,-0.99821,
		0.0014512,0.0029025,0.0014512,1.9955,-0.99551,
		0.0014512,-0.0029025,0.0014512,2.0005,-1.0005 
	};

	const float32_t coefficientsOctave63Hz[NUM_TAPS] = {
		0.0028997,-4.6561e-10,-0.0028997,1.994,-0.99403,
		0.0028997,0.0057993,0.0028996,1.9961,-0.99623,
		0.0028997,-0.0057993,0.0028996,1.9981,-0.99814 
	};

	const float32_t coefficientsOctave125Hz[NUM_TAPS] = {
		0.0057424,-9.2208e-10,-0.0057424,1.9883,-0.98854,
		0.0057424,0.011485,0.0057423,1.9921,-0.99256,
		0.0057424,-0.011485,0.0057423,1.9958,-0.99594 
	};

	const float32_t coefficientsOctave250Hz[NUM_TAPS] = {
		0.011441,-1.8372e-09,-0.011441,1.9761,-0.9772,
		0.011441,0.022882,0.011441,1.9832,-0.98518,
		0.011441,-0.022882,0.011441,1.9913,-0.99191 
	};

	const float32_t coefficientsOctave500Hz[NUM_TAPS] = {
		0.02271,-3.6467e-09,-0.02271,1.9507,-0.95492,
		0.02271,0.04542,0.02271,1.9628,-0.97058,
		0.02271,-0.04542,0.02271,1.9816,-0.98389 
	};

	const float32_t coefficientsOctave1000Hz[NUM_TAPS] = {
		0.04475,-7.1858e-09,-0.044751,1.8954,-0.91178,
		0.04475,0.089501,0.04475,1.9116,-0.94211,
		0.04475,-0.089501,0.04475,1.9588,-0.96799 
	};

	const float32_t coefficientsOctave2000Hz[NUM_TAPS] = {
		0.08697,-1.3965e-08,-0.086971,1.7681,-0.83067,
		0.08697,0.17394,0.086969,1.7703,-0.88818,
		0.08697,-0.17394,0.086969,1.9008,-0.93674 
	};

	const float32_t coefficientsOctave4000Hz[NUM_TAPS] = {
		0.16487,-2.6475e-08,-0.16488,1.4571,-0.68551,
		0.16487,0.32975,0.16487,1.3581,-0.79336,
		0.16487,-0.32975,0.16487,1.7372,-0.87536 
	};

	const float32_t coefficientsOctave8000Hz[NUM_TAPS] = {
		0.30049,-4.8251e-08,-0.30049,0.68723,-0.44235,
		0.30049,0.60098,0.30049,0.22883,-0.66257,
		0.30049,-0.60098,0.30049,1.2517,-0.74954 
	};

	const float32_t coefficientsOctave16000Hz[NUM_TAPS] = {
		0.524,-8.4141e-08,-0.52401,-0.85657,-0.047587,
		0.524,-1.048,0.524,0.076869,-0.39503,
		0.524,1.048,0.524,-1.8159,-0.8489 
	};

	const float32_t coefficientsWeighting[NUM_TAPS] = {
		0.2343,0.4686,0.2343,1.8939,-0.89516,
		1,-2.0001,1.0001,1.9946,-0.99462,
		1,-1.9999,0.99986,0.22456,-0.012607 
	};

static float32_t filtersStatus[FILTER_NUMBER][4*NUM_STAGES];

static arm_biquad_casd_df1_inst_f32 SFilters[FILTER_NUMBER];

float uint32ToFloat(uint32_t data){
	return ((float)data)/4095.0f;
}

void init_processing() {
	
	memcpy(filterCoefficients[0], 	coefficientsOctave31_5Hz, 	sizeof filterCoefficients[0]);
	memcpy(filterCoefficients[1], 	coefficientsOctave63Hz, 		sizeof filterCoefficients[1]);
	memcpy(filterCoefficients[2], 	coefficientsOctave125Hz, 		sizeof filterCoefficients[2]);
	memcpy(filterCoefficients[3], 	coefficientsOctave250Hz, 		sizeof filterCoefficients[3]);
	memcpy(filterCoefficients[4], 	coefficientsOctave500Hz, 		sizeof filterCoefficients[4]);
	memcpy(filterCoefficients[5], 	coefficientsOctave1000Hz, 	sizeof filterCoefficients[5]);
	memcpy(filterCoefficients[6], 	coefficientsOctave2000Hz, 	sizeof filterCoefficients[6]);
	memcpy(filterCoefficients[7], 	coefficientsOctave4000Hz, 	sizeof filterCoefficients[7]);
	memcpy(filterCoefficients[8], 	coefficientsOctave8000Hz, 	sizeof filterCoefficients[8]);
	memcpy(filterCoefficients[9], 	coefficientsOctave16000Hz,	sizeof filterCoefficients[9]);
	memcpy(filterCoefficients[10], 	coefficientsWeighting, 			sizeof filterCoefficients[10]);
	
	for (int i = 1; i < FILTER_NUMBER; i++) {
		arm_biquad_cascade_df1_init_f32(&SFilters[i], NUM_STAGES, (float32_t *)&filterCoefficients[i][0], filtersStatus[i]);
	}
}

void processfloat(float *inData, float *outData, int32_t lengthIn, int *arrayPosition){
	// RMS Calculation before applying any filter
	for (int j = 0; j < FRAME_LENGTH; j++) valueRMSTemp[0] += inData[j]*inData[j];
	
	// Filter
	for (int i = 1; i < FILTER_NUMBER; i++) {
		arm_biquad_cascade_reusable(&SFilters[i], inData , outDataFilter, lengthIn);
		for (int j = 0; j < FRAME_LENGTH; j++) valueRMSTemp[i+1] += outDataFilter[j]*outDataFilter[j];
	}
	
	// RMS Calculation after filters once the frame has been totally obtained 
	if (frameNumber == (FRAMES_PER_DATA_UNIT-1)){
		for (int i = 0; i < FILTER_NUMBER + 1; i++) {
			outData[i] = sqrt((valueRMSTemp[i]/(FRAME_LENGTH*FRAMES_PER_DATA_UNIT)))*3.3f;
			valueRMSTemp[i] = 0;
		}
		if (++(arrayPosition[0]) == NUMBER_OF_DATA_TO_SEND) arrayPosition[0] = 0;	
	}
	if (++frameNumber == FRAMES_PER_DATA_UNIT) frameNumber = 0;	
}


void process(uint32_t *inData, float *outData, int32_t length, int *arrayPosition) {
	for (int i = 0; i < length; i++) inDataFloat[i]=uint32ToFloat(inData[i]);
	processfloat(inDataFloat,outData, length, arrayPosition);
}

int getFrameIndex(void){
	return frameNumber;
}

void arm_biquad_cascade_reusable(const arm_biquad_casd_df1_inst_f32 * S, float32_t * pSrc, float32_t * pDst, uint32_t blockSize){

	pIn = pSrc;                         /*  source pointer            */
  pOut = pDst;                        /*  destination pointer       */
  pState = S->pState;                 /*  pState pointer            */
  pCoeffs = S->pCoeffs;               /*  coefficient pointer       */
  stage = S->numStages;         			/*  loop counters             */
		
#ifndef ARM_MATH_CM0_FAMILY

  /* Run the below code for Cortex-M4 and Cortex-M3 */

  do
  {
    /* Reading the coefficients */
    b0 = *pCoeffs++;
    b1 = *pCoeffs++;
    b2 = *pCoeffs++;
    a1 = *pCoeffs++;
    a2 = *pCoeffs++;

    /* Reading the pState values */
    Xn1 = pState[0];
    Xn2 = pState[1];
    Yn1 = pState[2];
    Yn2 = pState[3];

    /* Apply loop unrolling and compute 4 output values simultaneously. */
    /*      The variable acc hold output values that are being computed:    
     *    
     *    acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1]   + a2 * y[n-2]    
     *    acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1]   + a2 * y[n-2]    
     *    acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1]   + a2 * y[n-2]    
     *    acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1]   + a2 * y[n-2]    
     */

    sample = blockSize >> 2u;

    /* First part of the processing with loop unrolling.  Compute 4 outputs at a time.    
     ** a second loop below computes the remaining 1 to 3 samples. */
    while(sample > 0u)
    {
      /* Read the first input */

      Xn = *pIn++;

      /* acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2] */
      Yn2 = (b0 * Xn) + (b1 * Xn1) + (b2 * Xn2) + (a1 * Yn1) + (a2 * Yn2);

      /* Store the result in the accumulator in the destination buffer. */
      *pOut++ = Yn2;

      /* Every time after the output is computed state should be updated. */
      /* The states should be updated as:  */
      /* Xn2 = Xn1    */
      /* Xn1 = Xn     */
      /* Yn2 = Yn1    */
      /* Yn1 = acc   */

      /* Read the second input */
      Xn2 = *pIn++;


      /* acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2] */
      Yn1 = (b0 * Xn2) + (b1 * Xn) + (b2 * Xn1) + (a1 * Yn2) + (a2 * Yn1);


      /* Store the result in the accumulator in the destination buffer. */
      *pOut++ = Yn1;


      /* Every time after the output is computed state should be updated. */
      /* The states should be updated as:  */
      /* Xn2 = Xn1    */
      /* Xn1 = Xn     */
      /* Yn2 = Yn1    */
      /* Yn1 = acc   */

      /* Read the third input */
      Xn1 = *pIn++;


      /* acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2] */
      Yn2 = (b0 * Xn1) + (b1 * Xn2) + (b2 * Xn) + (a1 * Yn1) + (a2 * Yn2);


      /* Store the result in the accumulator in the destination buffer. */
      *pOut++ = Yn2;


      /* Every time after the output is computed state should be updated. */
      /* The states should be updated as: */
      /* Xn2 = Xn1    */
      /* Xn1 = Xn     */
      /* Yn2 = Yn1    */
      /* Yn1 = acc   */

      /* Read the forth input */
      Xn = *pIn++;


      /* acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2] */
      Yn1 = (b0 * Xn) + (b1 * Xn1) + (b2 * Xn2) + (a1 * Yn2) + (a2 * Yn1);


      /* Store the result in the accumulator in the destination buffer. */
      *pOut++ = Yn1;


      /* Every time after the output is computed state should be updated. */
      /* The states should be updated as:  */
      /* Xn2 = Xn1    */
      /* Xn1 = Xn     */
      /* Yn2 = Yn1    */
      /* Yn1 = acc   */
      Xn2 = Xn1;

      Xn1 = Xn;


      /* decrement the loop counter */
      sample--;

    }

    /* If the blockSize is not a multiple of 4, compute any remaining output samples here.    
     ** No loop unrolling is used. */
    sample = blockSize & 0x3u;

    while(sample > 0u)
    {
      /* Read the input */
      Xn = *pIn++;

      /* acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2] */
      acc = (b0 * Xn) + (b1 * Xn1) + (b2 * Xn2) + (a1 * Yn1) + (a2 * Yn2);

      /* Store the result in the accumulator in the destination buffer. */
      *pOut++ = acc;

      /* Every time after the output is computed state should be updated. */
      /* The states should be updated as:    */
      /* Xn2 = Xn1    */
      /* Xn1 = Xn     */
      /* Yn2 = Yn1    */
      /* Yn1 = acc   */
      Xn2 = Xn1;
      Xn1 = Xn;
      Yn2 = Yn1;
      Yn1 = acc;

      /* decrement the loop counter */
      sample--;

    }

    /*  Store the updated state variables back into the pState array */
    *pState++ = Xn1;
    *pState++ = Xn2;
    *pState++ = Yn1;
    *pState++ = Yn2;

    /*  The first stage goes from the input buffer to the output buffer. */
    /*  Subsequent numStages  occur in-place in the output buffer */

    pIn = pDst;

    /* Reset the output pointer */
    pOut = pDst;

    /* decrement the loop counter */
    stage--;

  } while(stage > 0u);

#endif /*   #ifndef ARM_MATH_CM0_FAMILY         */

}
