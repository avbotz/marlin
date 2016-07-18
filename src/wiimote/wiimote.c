/*
 *	wiiuse
 *
 *	Written By:
 *		Michael Laforest	< para >
 *		Email: < thepara (--AT--) g m a i l [--DOT--] com >
 *
 *	Copyright 2006-2007
 *
 *	This file is part of wiiuse.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	$Header$
 *
 */

/**
 *	@file
 *
 *	@brief Example using the wiiuse API.
 *
 *	This file is an example of how to use the wiiuse library.
 */

#include <stdio.h>                      /* for printf */
#include <stdlib.h>
#include <stdarg.h>
#include "wiiuse.h"                     /* for wiimote_t, classic_ctrl_t, etc */

#ifndef WIIUSE_WIN32
#include <unistd.h>                     /* for usleep */
#endif

#define MAX_WIIMOTES				4


/**
 *	@brief Callback that handles an event.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *
 *	This function is called automatically by the wiiuse library when an
 *	event occurs on the specified wiimote.
 */
const float X_SCALE = 1;
const float Y_SCALE = 1;
const float DEPTH_SCALE = 1;
const float YAW_SCALE = .25;
enum {X, Y, DEPTH, YAW, PITCH, ROLL};
float  desired_property[6] = {0};
float  current_property[6] = {0};

void get_state(){
	printf("c \n");
	fflush(stdout);
	float x, y, depth ,yaw, pitch, roll;
	fprintf(stderr, "Looking for input...");
	char f;
	do{
		f = fgetc(stdin);
		if (f == 's')
			scanf("%f %f %f %f %f %f", &x, &y, &depth, &yaw, &pitch, &roll);
	} while(f != 's');

	fprintf(stderr, "Recieved input.");
 
	current_property[X] = x;
	current_property[Y] = y;
	current_property[DEPTH] = depth;
	current_property[YAW] = yaw;
	current_property[PITCH] = pitch;
	current_property[ROLL] = roll;
}
 
void set_state(int index, float desire){
	desired_property[index] = current_property[index] + desire; 
}
	
void send_state() {
	printf("s s %f %f %f %f %f %f\n",
		desired_property[X],
		desired_property[Y],
		desired_property[DEPTH],
		desired_property[YAW],
		desired_property[PITCH],
		desired_property[ROLL]
	);
	fflush(stdout);

}
void handle_event(struct wiimote_t* wm) {
	get_state();
	for(int i = 0; i < 6; i++){
		desired_property[i] = 0;
	}
	/* if a button is pressed, report it */
	if (IS_PRESSED(wm, WIIMOTE_BUTTON_A)) {
	}
 
	if (IS_PRESSED(wm, WIIMOTE_BUTTON_B)) {
	}
 
	if (IS_PRESSED(wm, WIIMOTE_BUTTON_UP)) {
		//printf("strafing left\n");
		set_state(Y, -Y_SCALE);
	}
	if (IS_PRESSED(wm, WIIMOTE_BUTTON_DOWN))	{
		//printf("strafing right\n");
		set_state(Y, Y_SCALE);
	}
 
	if (IS_PRESSED(wm, WIIMOTE_BUTTON_LEFT))	{
		//printf("heaving down\n");
		set_state(DEPTH, DEPTH_SCALE);
	}
	if (IS_PRESSED(wm, WIIMOTE_BUTTON_RIGHT))	{
		//printf("heaving up\n");
		set_state(DEPTH, -DEPTH_SCALE);
	}
	if (IS_PRESSED(wm, WIIMOTE_BUTTON_MINUS))	{
		//printf("MINUS pressed\n");
	}
	if (IS_PRESSED(wm, WIIMOTE_BUTTON_PLUS))	{
		//printf("PLUS pressed\n");
	}
	if (IS_PRESSED(wm, WIIMOTE_BUTTON_ONE)) {
		//printf("surging backward\n");
		set_state(X, -X_SCALE);
	}
	if (IS_PRESSED(wm, WIIMOTE_BUTTON_TWO)) {
		//printf("surging forward\n");
		set_state(X, X_SCALE);
	}
	if (IS_PRESSED(wm, WIIMOTE_BUTTON_HOME))	{
		//printf("HOME pressed\n");
	}

	/*
	 *	Pressing minus will tell the wiimote we are no longer interested in movement.
	 *	This is useful because it saves battery power.
	 */
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_MINUS)) {
		fprintf(stderr, "wii accelerometer disabled \n");
		fflush(stderr);
		wiiuse_motion_sensing(wm, 0);
	}

	/*
	 *	Pressing plus will tell the wiimote we are interested in movement.
	 */
	if (IS_JUST_PRESSED(wm, WIIMOTE_BUTTON_PLUS)) {
		fprintf(stderr, "wii accelerometer enabled \n");
		fflush(stderr);
		wiiuse_motion_sensing(wm, 1);
	}

	/* if the accelerometer is turned on then print angles */
	if (WIIUSE_USING_ACC(wm)) {
		//printf("wiimote roll  = %f [%f]\n", wm->orient.roll, wm->orient.a_roll);
		//printf("wiimote pitch = %f [%f]\n", wm->orient.pitch, wm->orient.a_pitch);
		//printf("wiimote yaw   = %f\n", wm->orient.yaw); */
		if (wm->orient.pitch > 20) {
			//printf("turning left");
			set_state(YAW, -YAW_SCALE);}
		if (wm->orient.pitch < -20){
			//printf("turning right");
			set_state(YAW, YAW_SCALE);}
	}
	send_state();
}

void handle_read(struct wiimote_t* wm, byte* data, unsigned short len) {
	int i = 0;

	//printf("\n\n--- DATA READ [wiimote id %i] ---\n", wm->unid);
	//printf("finished read of size %i\n", len);
	for (; i < len; ++i) {
		if (!(i % 16)) {
			//printf("\n");
		}
		//printf("%x ", data[i]);
	}
	//printf("\n\n");
}


void handle_ctrl_status(struct wiimote_t* wm) {
	/*printf("\n\n--- CONTROLLER STATUS [wiimote id %i] ---\n", wm->unid);

	printf("attachment:      %i\n", wm->exp.type);
	printf("speaker:         %i\n", WIIUSE_USING_SPEAKER(wm));
	printf("ir:              %i\n", WIIUSE_USING_IR(wm));
	printf("leds:            %i %i %i %i\n", WIIUSE_IS_LED_SET(wm, 1), WIIUSE_IS_LED_SET(wm, 2), WIIUSE_IS_LED_SET(wm, 3), WIIUSE_IS_LED_SET(wm, 4));
	printf("battery:         %f %%\n", wm->battery_level);*/
}


void handle_disconnect(wiimote* wm) {
	//printf("\n\n--- DISCONNECTED [wiimote id %i] ---\n", wm->unid);
}


void test(struct wiimote_t* wm, byte* data, unsigned short len) {
	//printf("test: %i [%x %x %x %x]\n", len, data[0], data[1], data[2], data[3]);
}

short any_wiimote_connected(wiimote** wm, int wiimotes) {
	int i;
	if (!wm) {
		return 0;
	}

	for (i = 0; i < wiimotes; i++) {
		if (wm[i] && WIIMOTE_IS_CONNECTED(wm[i])) {
			return 1;
		}
	}

	return 0;
}
;
int main(int argc, char** argv) {
	wiimote** wiimotes;
	int found, connected;

	/*
	 *	Initialize an array of wiimote objects.
	 *
	 *	The parameter is the number of wiimotes I want to create.
	 */
	wiimotes =  wiiuse_init(MAX_WIIMOTES);

	/*
	 *	Find wiimote devices
	 *
	 *	Now we need to find some wiimotes.
	 *	Give the function the wiimote array we created, and tell it there
	 *	are MAX_WIIMOTES wiimotes we are interested in.
	 *
	 *	Set the timeout to be 5 seconds.
	 *
	 *	This will return the number of actual wiimotes that are in discovery mode.
	 */
	found = wiiuse_find(wiimotes, MAX_WIIMOTES, 5);
	if (!found) {
		fprintf(stderr, "No wiimotes found.\n");
		fflush(stdout);
		return 0;
	}

	/*
	 *	Connect to the wiimotes
	 *
	 *	Now that we found some wiimotes, connect to them.
	 *	Give the function the wiimote array and the number
	 *	of wiimote devices we found.
	 *
	 *	This will return the number of established connections to the found wiimotes.
	 */
	connected = wiiuse_connect(wiimotes, MAX_WIIMOTES);
	if (connected) {
		fprintf(stderr, "Connected to %i wiimotes (of %i found).\n", connected, found);
		fflush(stdout);
	} else {
		fprintf(stderr, "Failed to connect to any wiimote.\n");
 
		fflush(stdout);
		return 0;
	}

	/*
	 *	Now set the LEDs and rumble for a second so it's easy
	 *	to tell which wiimotes are connected (just like the wii does).
	 */
	wiiuse_set_leds(wiimotes[0], WIIMOTE_LED_1);
	wiiuse_set_leds(wiimotes[1], WIIMOTE_LED_2);
	wiiuse_set_leds(wiimotes[2], WIIMOTE_LED_3);
	wiiuse_set_leds(wiimotes[3], WIIMOTE_LED_4);
	wiiuse_rumble(wiimotes[0], 1);
	wiiuse_rumble(wiimotes[1], 1);

#ifndef WIIUSE_WIN32
	usleep(200000);
#else
	Sleep(200);
#endif

	wiiuse_rumble(wiimotes[0], 0);
	wiiuse_rumble(wiimotes[1], 0);
	//printf("hi");
	//fflush(stdout);
	fprintf(stderr, "\nControls:\n");
	fprintf(stderr, "\t '2' -> surge forwards\n");
	fprintf(stderr, "\t '1' -> surge backwards\n");
	fprintf(stderr, "\t 'up' -> heave upwards\n");
	fprintf(stderr, "\t 'down' -> heave downwards\n");
	fprintf(stderr, "\t 'left' -> strafe left\n");
	fprintf(stderr, "\t 'right' -> strafe right\n");
	fprintf(stderr, "\t+ to start Wiimote accelerometer , - to stop\n");
	fprintf(stderr, "\t while the accelerometer is on, tilt to turn\n");
	fprintf(stderr, "\n\n");
	fflush(stderr);

	/*
	 *	Maybe I'm interested in the battery power of the 0th
	 *	wiimote.  This should be WIIMOTE_ID_1 but to be sure
	 *	you can get the wiimote associated with WIIMOTE_ID_1
	 *	using the wiiuse_get_by_id() function.
	 *
	 *	A status request will return other things too, like
	 *	if any expansions are plugged into the wiimote or
	 *	what LEDs are lit.
	 */
	/* wiiuse_status(wiimotes[0]); */

	/*
	 *	This is the main loop
	 *
	 *	wiiuse_poll() needs to be called with the wiimote array
	 *	and the number of wiimote structures in that array
	 *	(it doesn't matter if some of those wiimotes are not used
	 *	or are not connected).
	 *
	 *	This function will set the event flag for each wiimote
	 *	when the wiimote has things to report.
	 */
	while (any_wiimote_connected(wiimotes, MAX_WIIMOTES)) {
		if (wiiuse_poll(wiimotes, MAX_WIIMOTES)) {
			/*
			 *	This happens if something happened on any wiimote.
			 *	So go through each one and check if anything happened.
			 */
			int i = 0;
			for (; i < MAX_WIIMOTES; ++i) {
				switch (wiimotes[i]->event) {
					case WIIUSE_EVENT:
						/* a generic event occurred */
						handle_event(wiimotes[i]);
						break;

					case WIIUSE_STATUS:
						/* a status event occurred */
						handle_ctrl_status(wiimotes[i]);
						break;

					case WIIUSE_DISCONNECT:
					case WIIUSE_UNEXPECTED_DISCONNECT:
						/* the wiimote disconnected */
						handle_disconnect(wiimotes[i]);
						break;

					case WIIUSE_READ_DATA:
						/*
						 *	Data we requested to read was returned.
						 *	Take a look at wiimotes[i]->read_req
						 *	for the data.
						 */
						break;

					default:
						break;
				}
			}
		}
	}

	/*
	 *	Disconnect the wiimotes
	 */
	wiiuse_cleanup(wiimotes, MAX_WIIMOTES);

	return 0;
}
