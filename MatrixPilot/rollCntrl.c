// This file is part of MatrixPilot.
//
//    http://code.google.com/p/gentlenav/
//
// Copyright 2009-2011 MatrixPilot Team
// See the AUTHORS.TXT file for a list of authors of MatrixPilot.
//
// MatrixPilot is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// MatrixPilot is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with MatrixPilot.  If not, see <http://www.gnu.org/licenses/>.


#include "defines.h"

#ifdef TEST_ACCX_STABILIZATION
#include "filters.h"
union int32_w2 accx_filt, accy_filt, accz_filt;
struct relative2D matrix_accum;
extern fractional gplane[];
int16_t roll_angle;
#endif

#if (USE_CONFIGFILE == 1)
#include "config.h"
#include "redef.h"

	uint16_t yawkdail;
	uint16_t rollkp;
	uint16_t rollkd;
#elif ((SERIAL_OUTPUT_FORMAT == SERIAL_MAVLINK) || (GAINS_VARIABLE == 1))
	uint16_t yawkdail			= (uint16_t)(YAWKD_AILERON*SCALEGYRO*RMAX);
	uint16_t rollkp				= (uint16_t)(ROLLKP*RMAX);
	uint16_t rollkd				= (uint16_t)(ROLLKD*SCALEGYRO*RMAX);
#else 
	const uint16_t yawkdail		= (uint16_t)(YAWKD_AILERON*SCALEGYRO*RMAX);
	const uint16_t rollkp		= (uint16_t)(ROLLKP*RMAX);
	const uint16_t rollkd		= (uint16_t)(ROLLKD*SCALEGYRO*RMAX);
#endif	

#if (USE_CONFIGFILE == 1)
	uint16_t hoverrollkp;
	uint16_t hoverrollkd;
#elif ((SERIAL_OUTPUT_FORMAT == SERIAL_MAVLINK) || (GAINS_VARIABLE == 1))
	uint16_t hoverrollkp		= (uint16_t)(HOVER_ROLLKP*SCALEGYRO*RMAX);
	uint16_t hoverrollkd		= (uint16_t)(HOVER_ROLLKD*SCALEGYRO*RMAX);
#else
	const uint16_t hoverrollkp	= (uint16_t)(HOVER_ROLLKP*SCALEGYRO*RMAX);
	const uint16_t hoverrollkd	= (uint16_t)(HOVER_ROLLKD*SCALEGYRO*RMAX);
#endif

void normalRollCntrl(void);
void hoverRollCntrl(void);

#if (USE_CONFIGFILE == 1)
void init_rollCntrl(void) {
	yawkdail 	= (uint16_t)(YAWKD_AILERON*SCALEGYRO*RMAX);
	rollkp 		= (uint16_t)(ROLLKP*RMAX);
	rollkd 		= (uint16_t)(ROLLKD*SCALEGYRO*RMAX);

	hoverrollkp = (uint16_t)(HOVER_ROLLKP*SCALEGYRO*RMAX);
	hoverrollkd = (uint16_t)(HOVER_ROLLKD*SCALEGYRO*RMAX);
}
#endif

void rollCntrl(void) {
    if (canStabilizeHover() && current_orientation == F_HOVER) {
		hoverRollCntrl();
    } else {
		normalRollCntrl();
	}
}

void normalRollCntrl(void) {
	union longww rollAccum = { 0 };
	union longww gyroRollFeedback;
	union longww gyroYawFeedback;

	fractional rmat6;
	fractional omegaAccum2;

    if (!canStabilizeInverted() || !desired_behavior._.inverted) {
		rmat6 = rmat[6];
		omegaAccum2 = omegaAccum[2];
    } else {
		rmat6 = -rmat[6];
		omegaAccum2 = -omegaAccum[2];
	}

#ifdef TestGains
	flags._.GPS_steering = 0; // turn off navigation
#endif
    // manual input is 2 * delta usec (range [-1000, 1000])
    int16_t roll_manual =  REVERSE_IF_NEEDED(AILERON_CHANNEL_REVERSED,
            (udb_pwIn[AILERON_INPUT_CHANNEL] - udb_pwTrim[AILERON_INPUT_CHANNEL]));

    // To convert roll_setpoint to a DCM angle, we need to scale it
    // to range from zero to +/- max. bank angle:
    // note that DX7 TX needs travel adjust +/-150% to achieve full PWM range
    // rmat ranges [-16384, 16383] for +/- 90 degrees
    roll_setpoint = (roll_manual << 4) + (roll_manual << 3);

    if (AILERON_NAVIGATION && flags._.GPS_steering) {
        // add to manual angle setpoint
        roll_setpoint += determine_navigation_deflection('a') << 3;
    }
    // limit combined manual and nav roll setpoint to less than +/-90 degrees
    magClamp(&roll_setpoint, 16000);

#ifdef TestGains
	flags._.pitch_feedback = 1;
#endif

#ifdef TEST_ACCX_STABILIZATION
    // test some simple ideas for flight stabilization
    // 1) force average accel vector parallel to body Z axis
    // 2) and force yaw rate to zero
    if (ROLL_STABILIZATION_AILERONS && flags._.pitch_feedback) {
        // compare roll setpoint to filtered body frame Z acceleration
        // instead of DCM roll angle. The roll angle is approximately
        // atan2(accx, accz)

        // lowpass filter the x,y,z accelerometer samples
        // The MPU6000 applies a 42Hz digital lowpass filter, but we probably
        // want just a few Hz of bandwidth for the accelerometer readings.
        // Note that this is executed at HEARTBEAT_HZ = 200, so the 3dB point
        // for lp2 with LPCB_45_HZ will be 4.5Hz
        lp2(gplane[1], &accy_filt, LPCB_45_HZ);
        matrix_accum.y = -lp2(gplane[0], &accx_filt, LPCB_45_HZ);
        matrix_accum.x = lp2(gplane[2], &accz_filt, LPCB_45_HZ);
        
        // binary angle (0 - 65536 = 360 degrees) or [-32K, 32K) -> [-180, 180) degrees
        // therefore 90 degrees is 16K = DCM sin(90)
        roll_angle = rect_to_polar16(&matrix_accum);

        // looks like double the gain is needed here
        rollAccum.WW = __builtin_mulsu((roll_setpoint - roll_angle) << 1, rollkp);

        // normal roll damping
		gyroRollFeedback.WW = __builtin_mulus(rollkd , omegaAccum[1]);
        gyroYawFeedback.WW = 0;
    } else {
        // no stabilization; pass manual input through
        rollAccum._.W1 = roll_manual;
        gyroRollFeedback.WW = 0;
        gyroYawFeedback.WW = 0;
	}
#else
        // control roll rate proportional to angle setpoint error
        if (ROLL_STABILIZATION_AILERONS && flags._.pitch_feedback) {
            // Beware: -rmat6 is roll angle, so it must be added here
            // NOT THE SAME as pitch which is (pitch_setpoint - rmat7)
            rollAccum.WW = 0;
            int16_t desRate = roll_setpoint + rmat6;
            gyroRollFeedback.WW = __builtin_mulus(rollkd, omegaAccum[1] - desRate);
        } else {
            // no stabilization; pass manual input through
            rollAccum._.W1 = roll_manual;
            gyroRollFeedback.WW = 0;
        }

        if (YAW_STABILIZATION_AILERON && flags._.pitch_feedback) {
                    gyroYawFeedback.WW = __builtin_mulus(yawkdail, omegaAccum2);
        } else {
                    gyroYawFeedback.WW = 0;
        }
#endif

    roll_control = rollAccum._.W1 - (int32_t) gyroRollFeedback._.W1 - (int32_t) gyroYawFeedback._.W1;
	// Servo reversing is handled in servoMix.c
}


void hoverRollCntrl(void) {
	int16_t rollNavDeflection;
	union longww gyroRollFeedback;

    if (flags._.pitch_feedback) {
        if (AILERON_NAVIGATION && flags._.GPS_steering) {
			rollNavDeflection = (tofinish_line > HOVER_NAV_MAX_PITCH_RADIUS/2) ? determine_navigation_deflection('h') : 0;
        } else {
			rollNavDeflection = 0;
		}
		
		gyroRollFeedback.WW = __builtin_mulus(hoverrollkd , omegaAccum[1]);
    } else {
		rollNavDeflection = 0;
		gyroRollFeedback.WW = 0;
	}

	roll_control = rollNavDeflection -(int32_t)gyroRollFeedback._.W1;
}
