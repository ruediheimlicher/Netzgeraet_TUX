/*
 * Hardware settings for the digital dc power supply
 * http://www.tuxgraphics.org/electronics/
 *
 * In this file you can:
 * - calibrate the ampere and voltmeter
 * - choose your hardware type: 22V 2.5A or 30V 2.0A
 *
 *   The ADC has a resolution of 11 bit = values from 0-2047
 */
#ifndef CAL_HW_H
#define CAL_HW_H


/* ================= uncomment this section for the model 22V 2.5A */

#define U_MAX 220
#define I_MAX 250

// internal adc ref voltage (should be 2.56V, can vary from uC to uC)
#define ADC_REF 2.56

// the divider R3/R4 [(R3+R4)/R4] (calibrate the voltmeter, lower value=higher output)
#define U_DIVIDER 10.75

// the shunt for current measurement, you can calibrate here the 
// amperemeter.
// 2*1.5Ohm 3W=0.75:
#define I_RESISTOR 0.79
// short circuit protection limit (do not change unless you know what you do):
// 850=2.85A= (2.85A * 1023 * 0.75 / 2.56 )
#define SH_CIR_PROT 850

/* ================= uncomment this section for the model 30V 2.0A */


//#define U_MAX 300
//#define I_MAX 200

// internal adc ref voltage (should be 2.56V, can vary from uC to uC)
//#define ADC_REF 2.56

// the divider R3/R4 [(R3+R4)/R4], you can calibrate here the voltmeter:
//#define U_DIVIDER 13.24

// the shunt for current measurement, you can calibrate here the 
// amperemeter.
// 2*1.5Ohm 3W=0.75:
//#define I_RESISTOR 0.78

// short circuit protection limit (do not change unless you know what you do):
// 690=2.30A= (2.30A * 1023 * 0.75 / 2.56 )
//#define SH_CIR_PROT 690

#endif //CAL_HW_H

