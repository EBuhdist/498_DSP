#include <std.h>
#include <stdbool.h>
#include <log.h>

#include "hellocfg.h"
#include "ezdsp5502.h"
#include "stdint.h"
#include "aic3204.h"
#include "ezdsp5502_mcbsp.h"
#include "ezdsp5502_i2cgpio.h"			// for the SW0 and SW1 macro definitions
#include "csl_mcbsp.h"

#include "myfir.h"
#include "nco.h"

extern MCBSP_Handle aicMcbsp;
extern int16_t switchInterrupt;
extern int16_t switchCount;
extern bool btn_press1;
extern bool btn_press2;

extern void myfir(int16_t *in, int16_t *delayLine, int16_t *out, uint16_t numOfSamps, uint16_t numOfCoeffs,const int16_t * filterCoeffs);

/* Global Vars for Processing */

//LPF_50-1150 Hz -- 55dB att -- order 80, 81 coeffs
const int16_t myLPF[81] =
{
        86,     39,     48,     57,     67,     79,     91,    105,    119,    135,    151,    169,    187,    206,    226,    247,
       268,    289,    311,    334,    356,    379,    401,    424,    446,    467,    488,    508,    527,    546,    563,    579,
       593,    606,    618,    628,    636,    643,    647,    650,    651,    650,    647,    643,    636,    628,    618,    606,
       593,    579,    563,    546,    527,    508,    488,    467,    446,    424,    401,    379,    356,    334,    311,    289,
       268,    247,    226,    206,    187,    169,    151,    135,    119,    105,     91,     79,     67,     57,     48,     39,
        86,
};
//const int16_t myLPF[61] =
//{
// 		-4,   -7,    -12,    -19,    -28,    -38,    -50,    -61,    -71,    -77,    -78,    -70,    -51,    -18,     33,    102,
//        192,    304,    437,    589,    757,    938,   1126,   1315,   1498,   1668,   1819,   1943,   2036,   2094,   2114,   2094,
//        2036,   1943,   1819,   1668,   1498,   1315,   1126,    938,    757,    589,    437,    304,    192,    102,     33,    -18,
//       -51,    -70,    -78,    -77,    -71,    -61,    -50,    -38,    -28,    -19,    -12,     -7,     -4
//};
int16_t delayLine[60];
int16_t dataIn;

//FOR PROJECT 3......TSK
int16_t inBuffer[96];
int16_t outBuffer[96];
uint16_t inCount = 0;
uint16_t outCount = 0;
bool fillTop = true;
bool fillReadTop = true;

//Previously for project 2...nco
int16_t leftBuffer[96];
int16_t rightBuffer[96];
int16_t leftOut[96];
int16_t rightOut[96];
uint16_t leftCount = 0;
uint16_t rightCount = 0;
uint16_t leftCountOut = 0;
uint16_t rightCountOut = 0;
uint16_t softCount = 0;
bool useLeft = true;
bool useLeftOut = true;
bool softIndex = true;

void audioProcessingInit(void) {
	dataIn = 0;
	memset(delayLine, 0, sizeof(delayLine));
	memset(leftOut, 0, sizeof(outBuffer));
	memset(rightOut, 0, sizeof(rightOut));
}

void SWI_AudioProc(void) {
//
//	if (softIndex == true) {
//
//		myfir((int16_t *) &rightBuffer[0], delayLine, (int16_t *) &rightOut[0], 48, 81, myLPF);
//
//		for (softCount = 0; softCount < 48; softCount++) {
//
//			leftOut[softCount] = leftBuffer[softCount];
//		}
//	}
//
//	else if (softIndex == false) {
//
//		myfir((int16_t *) &rightBuffer[48], delayLine, (int16_t *) &rightOut[48], 48, 81, myLPF);
//
//		for (softCount = 48; softCount < 96; softCount++) {
//
//			leftOut[softCount] = leftBuffer[softCount];
//		}
//
//	}
//
}

void HWI_I2S_Rx(void) {

	EZDSP5502_MCBSP_read((Int16*) &dataIn);
	/*
	 * Need to reorganize this HWI. The audio TSK will do all of the grinding in through the mailbox.
	 * Mailbox needs to pend and post only one time, so a 96 buffer is necessary to use.
	 * To make the filter implementation easier later on we should consider sorting the left half in the top and the
	 * 		right half in the bottom of the array. That way, in the TSK, we can just give the lower 48 to memory (the right
	 * 		channel), and have it filter those instead of trying to figure out which is which. That being said, I think that we
	 * 		could potentially use a for loop like in the SWI to do the same thing and just dictate which loop it will go into if the
	 * 		button is pressed or not.
	 *
	 * TODO: the HWI_Rx needs to be modified to take in one [96] buffer. Then post to the mailbox once, and reset. Buffers can be immediately
	 * 		reset after they fill a mailbox.
	 *
	 * TODO: note that inBuffer[96] and outBuffer[96] have been made to store these already. Look above in global variables. The variables stored for
	 * 		Project 2 (NCO implementation) are being kept for build purposes, we can delete them later or as we go to clean up and free memory.
	 */
//fill top half with left and bottom with right, one 96 buffer to a mailbox, post and pend once

	if (fillTop == true) {

		inBuffer[inCount] = dataIn;
		fillTop = false;
	}

	else {

		inBuffer[inCount+48] = dataIn;
		inCount++;
		fillTop = true;


		if (inCount == 48) {

			MBX_post(&audioMBX, inBuffer, 0);
			inCount = 0;
		}
	}
}

void HWI_I2S_Tx(void) {

		if (fillReadTop == true) {

			EZDSP5502_MCBSP_write(outBuffer[outCount]);
			//outCount++;
			fillReadTop = false;
		}

		else {

			EZDSP5502_MCBSP_write(outBuffer[outCount+48]);
			outCount++;
			fillReadTop = true;

			if (outCount == 48) {

				outCount = 0;
			}
		}

//	}
}

void TSK_audio(void) {
/*
 * Single buffer has been implemented here. This is simply just a pass through version of
 *		the audio TSK, for testing. I think the dual mailbox pend was causing issues and it
 *		was no longer RT. I dont think any further modifications here will be necessary. Take a look
 *		though, I am wrong often.
 *
 * NOTE: that the SYS_FOREVER variable is literally just a -1 that has been typecasted. So we can always
 *		use -1 instead of that stupid phrase.
 */

	int16_t tempBuffer[96];
	int index = 0;

	while(1) {

		MBX_pend(&audioMBX, tempBuffer, SYS_FOREVER);

		// If button is pressed then filter both channels
		if (btn_press1 == 1) {

			for(index = 0; index < 48; index++) {

				/*
				 * Filter both channels  Left (0-47 samples) Right (48-95 Samples)
				 */
				myfir((int16_t*)&tempBuffer[index], delayLine, (int16_t*)&outBuffer[index], 1, 81, myLPF);
				myfir((int16_t*)&tempBuffer[index+48], delayLine, (int16_t*)&outBuffer[index+48], 1, 81, myLPF);
			}
		} else {
				/*
				 * Don't filter either channel
				 */
				for(index = 0; index < 48; index++) {
					outBuffer[index] = tempBuffer[index];
					outBuffer[index+48] = tempBuffer[index+48];
				}
		}
	}
}

void TSK_button(void) {

	// Intro stuff

	while(1) {

		if (EZDSP5502_I2CGPIO_readLine(SW0) == 0) {
			btn_press1 = btn_press1 ^ 1; // XOR toggles the btn_press variable
		}

		if (EZDSP5502_I2CGPIO_readLine(SW1) == 0) {
			btn_press2 = btn_press2 ^ 1; // XOR toggles the btn_press variable
			if (btn_press2 == 1) {
				EZDSP5502_I2CGPIO_writeLine(   LED0, HIGH ); // if btn is pressed turn on LED
			} else {
				EZDSP5502_I2CGPIO_writeLine(   LED0, LOW ); // if btn is not pressed turn off LED
			}
		}
	}

}

void TSK_fft(void) {

	// Intro stuff

//	while(1) {
//
//	}

}
