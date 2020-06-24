#ifndef INC_PROCESSING_H_
#define INC_PROCESSING_H_



#endif /* INC_PROCESSING_H_ */
#ifdef __cplusplus
extern "C" {
#endif
#include "stm32f4xx.h"
#include "arm_math.h"
#include "config.h"

#define NUMBER_OF_DATA_TO_SEND	(int)(SECS_BETWEEN_WIFI_DUMP/((FRAME_LENGTH*FRAMES_PER_DATA_UNIT/48000.0)))						

float uint32ToFloat(uint32_t data);
void init_processing(void);
void processfloat(float *inData,float *outData,int32_t length, int *arrayPosition);
void process(uint32_t *inData, float *outData,int32_t length, int *arrayPosition);
void arm_biquad_cascade_reusable(const arm_biquad_casd_df1_inst_f32 * S, float32_t * pSrc, float32_t * pDst, uint32_t blockSize);
int getFrameIndex(void);

#ifdef __cplusplus
}
#endif
