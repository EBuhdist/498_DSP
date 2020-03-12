#include <std.h>

#include <log.h>
#include <clk.h>
#include <tsk.h>
#include <gbl.h>
//#include "clkcfg.h"

#include "hellocfg.h"
#include "ezdsp5502.h"
#include "stdint.h"
#include "aic3204.h"
#include "ezdsp5502_mcbsp.h"
#include "csl_mcbsp.h"
#include "ezdsp5502_i2cgpio.h"
#include "csl_gpio.h"
#include "nco.h"
extern void audioProcessingInit(void);
extern void ConfigureAic3204(void);

int16_t switchInterrupt = 0;
int counter = 0;
int counterTwo = 0;

void main(void)
 {
	/* Initialize BSL */
    EZDSP5502_init( );

    /* configure the Codec chip */
    ConfigureAic3204();

    /* Initialize I2S */
    EZDSP5502_MCBSP_init();

    /* Setup I2C GPIOs for Switches */
    EZDSP5502_I2CGPIO_configLine(  SW0, IN );
    EZDSP5502_I2CGPIO_configLine(  SW1, IN );

    /* enable the interrupt with BIOS call */
    C55_enableInt(7); // reference technical manual, I2S2 tx interrupt
    C55_enableInt(6); // reference technical manual, I2S2 rx interrupt

    audioProcessingInit();

    // after main() exits the DSP/BIOS scheduler starts
}

void myIDLThread(void){

   if(EZDSP5502_I2CGPIO_readLine(SW0) == 0 && counter > 10) {
    	switchInterrupt = 1;
        EZDSP5502_waitusec( 1000000 );

    }
    else {
    	counter++;
    	switchInterrupt = 0;
    }

    if(EZDSP5502_I2CGPIO_readLine(SW1) == 0 && counterTwo > 10) {
    	switchInterrupt = 2;
        EZDSP5502_waitusec( 1000000 );
    }
    else {
    	counterTwo++;
    	switchInterrupt = 0;
    }
}

#if 0
Void taskFxn(Arg value_arg)
{
    LgUns prevHtime, currHtime;
    uint32_t delta;
    float ncycles;

    /* get cpu cycles per htime count */
    ncycles = CLK_cpuCyclesPerHtime();

    while(1)
    {
        TSK_sleep(1);
        LOG_printf(&trace, "task running! Time is: %d ticks", (Int)TSK_time());

        prevHtime = currHtime;
        currHtime = CLK_gethtime();

        delta = (currHtime - prevHtime) * ncycles;
        LOG_printf(&trace, "CPU cycles = 0x%x %x", (uint16_t)(delta >> 16), (uint16_t)(delta));

    }
}
#endif


