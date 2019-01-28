#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "thruster-control.h"
#include "pca9685.h"

//Setup Registers
#define PCA9685_MODE1 0x0
#define PCA9685_PRESCALE 0xFE

/**
 * This entire program is devoted to a single thruster, as specified by the value passed to argv[argc].
 * This allows thruster control to be asynchronous and makes spawning new thruter programs pretty easy.
 */
int main(int argc, char* argv[]) {
    if(argc != 2) {
        perror("Wrong amount of arguments!\n");
        exit(1);
    }

    populate_whichami(argv[argc]);
    if(Whichami.data_source == -1) {
        perror("Didn't recognize that thruster\n");
        exit(2);
    }

    while(true) {
        int thruster_goal_value = comms_get_int(Whichami.data_source);
        bool error = false;
        error = do_thruster_movement(thruster_goal_value);
        
        if(error) {
            char string[75];
            sprintf(string, "Catastrophic falure of some kind, probably. (thuster %i)\n", Whichami.data_send);
            perror(string);
        }
    }
}

/**
 * Does the thruster movement. The value provided gets sent to Whichami.data_send.
 * @param goalval The goal value.
 * @return true on success, false on fail.
 */
//PWM max = 4095
bool do_thruster_movement(double goalval) {
    // goal value= percent pressed forward on joystick... speed
    //.h file that takes the goalval and translates into a pwm for speed
    if ( -1 < goalval && goalval < 1)
        double pwm;
        if(goalval > 0){ //goes "forward"
            pwm = (goalval*4095);
            myPwmWrite(Whichami.fd, pwm);
            //may need to check that it actually wrote the corect valur for troubleshooting
            return true;
        }
        else{  //goes "backward"
            pwm = (goalval*4095*(-1));
            myPwmWrite(Whichami.fd, pwm);
            //msy need to add in reading
            return true;
        }
    else
        return false;
}

/**
 * Populates the 'Whichami' struct.
 */
void populate_whichami(char* input) {
    strcpy(Whichami.name, input);

    if(!strcmp(input, "T_H_FRONTLEFT")) {
        Whichami.data_source = PORT_T_H_FRONTLEFT;
        Whichami.data_send   = T_H_FRONTLEFT;
        fd = 5; //need to change to actual values when electrical gives them to us
        return;
    }

    if(!strcmp(input, "T_H_FRONTRIGHT")) {
        Whichami.data_source = PORT_T_H_FRONTRIGHT;
        Whichami.data_send   = T_H_FRONTRIGHT;
        fd = 6; //need to change to actual values when electrical gives them to us
        return;
    }
   
    if(!strcmp(input, "T_H_BACKLEFT")) {
        Whichami.data_source = PORT_T_H_BACKLEFT;
        Whichami.data_send   = T_H_BACKLEFT;
        fd = 7; //need to change to actual values when electrical gives them to us
        return;
    }
   
    if(!strcmp(input, "T_H_BACKRIGHT")) {
        Whichami.data_source = PORT_T_H_BACKRIGHT;
        Whichami.data_send   = T_H_BACKRIGHT;
        fd = 8; //need to change to actual values when electrical gives them to us
        return;
    }
   
    if(!strcmp(input, "T_V_LEFT")) {
        Whichami.data_source = PORT_T_V_LEFT;
        Whichami.data_send   = T_V_LEFT;
        fd = 4; //need to change to actual values when electrical gives them to us
        return;
    }
   
    if(!strcmp(input, "T_V_RIGHT")) {
        Whichami.data_source = PORT_T_V_RIGHT;
        Whichami.data_send   = T_V_RIGHT;
        fd = 2; //need to change to actual values when electrical gives them to us
        return;
    }
   
    if(!strcmp(input, "T_V_FRONT")) {
        Whichami.data_source = PORT_T_V_FRONT;
        Whichami.data_send   = T_V_FRONT;
        fd = 1; //need to change to actual values when electrical gives them to us
        return;
    }
   
    if(!strcmp(input, "T_V_BACK")) {
        Whichami.data_source = PORT_T_V_BACK;
        Whichami.data_send   = T_V_BACK;
        fd = 3; //need to change to actual values when electrical gives them to us
        return;
    }

    // If we got here, there was no match... populate -1 so main can handle it.
    Whichami.data_source = -1;
    Whichami.data_send   = -1;
}
                
                
/**
 * Setup a PCA9685 device with wiringPi.
 *  
 * pinBase: 	Use a pinBase > 64, eg. 300
 * i2cAddress:	The default address is 0x40
 * freq:		Frequency will be capped to range [40..1000] Hertz. Try 50 for servos 
 */

int pca9685Setup(const int pinBase, const int i2cAddress, float freq)
{
	// Create a node with 16 pins [0..15] + [16] for all
	struct wiringPiNodeStruct *node = wiringPiNewNode(pinBase, PIN_ALL + 1);

	// Check if pinBase is available
	if (!node)
		return -1;

	// Check i2c address
	int fd = wiringPiI2CSetup(i2cAddress);
	if (fd < 0)
		return fd;

	// Setup the chip. Enable auto-increment of registers.
	int settings = wiringPiI2CReadReg8(fd, PCA9685_MODE1) & 0x7F;
	int autoInc = settings | 0x20;

	wiringPiI2CWriteReg8(fd, PCA9685_MODE1, autoInc);
	
	// Set frequency of PWM signals. Also ends sleep mode and starts PWM output.
	if (freq > 0)
		pca9685PWMFreq(fd, freq);
	

	node->fd			= fd;
	node->pwmWrite		= myPwmWrite;
	node->digitalWrite	= myOnOffWrite;
	node->digitalRead	= myOffRead;
	node->analogRead	= myOnRead;

	return fd;
}

/**
 * Sets the frequency of PWM signals.
 * Frequency will be capped to range [40..1000] Hertz. Try 50 for servos.
 */
void pca9685PWMFreq(int fd, float freq)
{
	// Cap at min and max
	freq = (freq > 1000 ? 1000 : (freq < 40 ? 40 : freq));

	// To set pwm frequency we have to set the prescale register. The formula is:
	// prescale = round(osc_clock / (4096 * frequency))) - 1 where osc_clock = 25 MHz
	// Further info here: http://www.nxp.com/documents/data_sheet/PCA9685.pdf Page 24
	int prescale = (int)(25000000.0f / (4096 * freq) - 0.5f);

	// Get settings and calc bytes for the different states.
	int settings = wiringPiI2CReadReg8(fd, PCA9685_MODE1) & 0x7F;	// Set restart bit to 0
	int sleep	= settings | 0x10;									// Set sleep bit to 1
	int wake 	= settings & 0xEF;									// Set sleep bit to 0
	int restart = wake | 0x80;										// Set restart bit to 1

	// Go to sleep, set prescale and wake up again.
	wiringPiI2CWriteReg8(fd, PCA9685_MODE1, sleep);
	wiringPiI2CWriteReg8(fd, PCA9685_PRESCALE, prescale);
	wiringPiI2CWriteReg8(fd, PCA9685_MODE1, wake);

	// Now wait a millisecond until oscillator finished stabilizing and restart PWM.
	delay(1);
	wiringPiI2CWriteReg8(fd, PCA9685_MODE1, restart);
}

/**
 * Set all leds back to default values (: fullOff = 1)
 */
void pca9685PWMReset(int fd)
{
	wiringPiI2CWriteReg16(fd, LEDALL_ON_L	 , 0x0);
	wiringPiI2CWriteReg16(fd, LEDALL_ON_L + 2, 0x1000);
}

/**
 * Write on and off ticks manually to a pin
 * (Deactivates any full-on and full-off)
 */
void pca9685PWMWrite(int fd, int pin, int on, int off)
{
	int reg = baseReg(pin);

	// Write to on and off registers and mask the 12 lowest bits of data to overwrite full-on and off
	wiringPiI2CWriteReg16(fd, reg	 , on  & 0x0FFF);
	wiringPiI2CWriteReg16(fd, reg + 2, off & 0x0FFF);
}

/**
 * Reads both on and off registers as 16 bit of data
 * To get PWM: mask each value with 0xFFF
 * To get full-on or off bit: mask with 0x1000
 * Note: ALL_LED pin will always return 0
 */
void pca9685PWMRead(int fd, int pin, int *on, int *off)
{
	int reg = baseReg(pin);

	if (on)
		*on  = wiringPiI2CReadReg16(fd, reg);
	if (off)
		*off = wiringPiI2CReadReg16(fd, reg + 2);
}

/**
 * Enables or deactivates full-on
 * tf = true: full-on
 * tf = false: according to PWM
 */
void pca9685FullOn(int fd, int pin, int tf)
{
	int reg = baseReg(pin) + 1;		// LEDX_ON_H
	int state = wiringPiI2CReadReg8(fd, reg);

	// Set bit 4 to 1 or 0 accordingly
	state = tf ? (state | 0x10) : (state & 0xEF);

	wiringPiI2CWriteReg8(fd, reg, state);

	// For simplicity, we set full-off to 0 because it has priority over full-on
	if (tf)
		pca9685FullOff(fd, pin, 0);
}

/**
 * Enables or deactivates full-off
 * tf = true: full-off
 * tf = false: according to PWM or full-on
 */
void pca9685FullOff(int fd, int pin, int tf)
{
	int reg = baseReg(pin) + 3;		// LEDX_OFF_H
	int state = wiringPiI2CReadReg8(fd, reg);

	// Set bit 4 to 1 or 0 accordingly
	state = tf ? (state | 0x10) : (state & 0xEF);

	wiringPiI2CWriteReg8(fd, reg, state);
}

/**
 * Helper function to get to register
 */
int baseReg(int pin)
{
	return (pin >= PIN_ALL ? LEDALL_ON_L : LED0_ON_L + 4 * pin);
}
*/
 
 /**
 * Simple PWM control which sets on-tick to 0 and off-tick to value.
 * If value is <= 0, full-off will be enabled
 * If value is >= 4096, full-on will be enabled
 * Every value in between enables PWM output
 */
static void myPwmWrite(struct wiringPiNodeStruct *node, int pin, int value)
{
	int fd   = node->fd;
	int ipin = pin - node->pinBase;

	if (value >= 4096)
		pca9685FullOn(fd, ipin, 1);
	else if (value > 0)
		pca9685PWMWrite(fd, ipin, 0, value);	// (Deactivates full-on and off by itself)
	else
		pca9685FullOff(fd, ipin, 1);
}

/**
 * Simple full-on and full-off control
 * If value is 0, full-off will be enabled
 * If value is not 0, full-on will be enabled
 */
static void myOnOffWrite(struct wiringPiNodeStruct *node, int pin, int value)
{
	int fd   = node->fd;
	int ipin = pin - node->pinBase;

	if (value)
		pca9685FullOn(fd, ipin, 1);
	else
		pca9685FullOff(fd, ipin, 1);
}

/**
 * Reads off registers as 16 bit of data
 * To get PWM: mask with 0xFFF
 * To get full-off bit: mask with 0x1000
 * Note: ALL_LED pin will always return 0
 */
static int myOffRead(struct wiringPiNodeStruct *node, int pin)
{
	int fd   = node->fd;
	int ipin = pin - node->pinBase;

	int off;
	pca9685PWMRead(fd, ipin, 0, &off);

	return off;
}

/**
 * Reads on registers as 16 bit of data
 * To get PWM: mask with 0xFFF
 * To get full-on bit: mask with 0x1000
 * Note: ALL_LED pin will always return 0
 */
static int myOnRead(struct wiringPiNodeStruct *node, int pin)
{
	int fd   = node->fd;
	int ipin = pin - node->pinBase;

	int on;
	pca9685PWMRead(fd, ipin, &on, 0);

	return on;
}
