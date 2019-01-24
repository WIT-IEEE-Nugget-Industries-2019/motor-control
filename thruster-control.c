#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "thruster-control.h"

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
bool do_thruster_movement(int goalval) {
    // goal value= percent pressed forward on joystick... speed
    //.h file that takes the goalval and translates into a pwm for speed
    if ( 0 < goalval && goalval < 1)
        //translate to pwm here
        return true;
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
        return;
    }

    if(!strcmp(input, "T_H_FRONTRIGHT")) {
        Whichami.data_source = PORT_T_H_FRONTRIGHT;
        Whichami.data_send   = T_H_FRONTRIGHT;
        return;
    }
   
    if(!strcmp(input, "T_H_BACKLEFT")) {
        Whichami.data_source = PORT_T_H_BACKLEFT;
        Whichami.data_send   = T_H_BACKLEFT;
        return;
    }
   
    if(!strcmp(input, "T_H_BACKRIGHT")) {
        Whichami.data_source = PORT_T_H_BACKRIGHT;
        Whichami.data_send   = T_H_BACKRIGHT;
        return;
    }
   
    if(!strcmp(input, "T_V_LEFT")) {
        Whichami.data_source = PORT_T_V_LEFT;
        Whichami.data_send   = T_V_LEFT;
        return;
    }
   
    if(!strcmp(input, "T_V_RIGHT")) {
        Whichami.data_source = PORT_T_V_RIGHT;
        Whichami.data_send   = T_V_RIGHT;
        return;
    }
   
    if(!strcmp(input, "T_V_FRONT")) {
        Whichami.data_source = PORT_T_V_FRONT;
        Whichami.data_send   = T_V_FRONT;
        return;
    }
   
    if(!strcmp(input, "T_H_FRONTLEFT")) {
        Whichami.data_source = PORT_T_H_FRONTLEFT;
        Whichami.data_send   = T_H_FRONTLEFT;
        return;
    }

    // If we got here, there was no match... populate -1 so main can handle it.
    Whichami.data_source = -1;
    Whichami.data_send   = -1;
}