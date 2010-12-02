// SEE END OF FILE FOR LICENSE TERMS

#include "../libUDB/libUDB_internal.h"
#include "CANInterface_defines.H"

/*
_FOSC( CSW_FSCM_OFF & HS ) ;		// external high speed crystal
_FWDT( WDT_OFF ) ;					// no watchdog timer
_FBORPOR( 	PBOR_OFF &				// brown out detection off
			MCLR_EN &				// enable MCLR
			RST_PWMPIN &			// pwm pins as pwm
			PWMxH_ACT_HI &			// PWMH is active high
			PWMxL_ACT_HI ) ;		// PMWL is active high
_FGS( CODE_PROT_OFF ) ;				// no protection
_FICD( 0xC003 ) ;					// normal use of debugging port
*/


_FOSC(CSW_FSCM_OFF & XT_PLL4);		// 16MHz clock with 4MHz crystal or resonator
//_FWDT( WDT_OFF ) ;				// no watchdog timer
_FWDT(WDT_ON & WDTPSA_8 & WDTPSB_12);
_FBORPOR( 	PBOR_OFF & // brown out detection off
			//PBOR_ON & // brown out detection on
			//BORV_27 & // brown out triggered at 2.7V
			//PWRT_64 & // powerup timer at 64ms
			MCLR_EN &  // enable MCLR
			RST_PWMPIN & // pwm pins as pwm
			PWMxH_ACT_HI & // PWMH is active high
			PWMxL_ACT_HI 
						) ; // PMWL is active high
_FGS( CODE_PROT_OFF ) ; // no protection
//_FICD( 0xC003 ) ;		// normal use of debugging port
_FICD( ICS_PGD ) ;		// normal use of debugging port




union udb_fbts_byte udb_flags ;

boolean timer_5_on = 0 ;
boolean needSaveExtendedState = 0 ;
int defaultCorcon = 0 ;


void udb_init(void)
{
	defaultCorcon = CORCON ;

	udb_flags.B = 0 ;
	
//	udb_init_leds() ;
	CAN_init();
//	udb_init_ADC() ;
	udb_init_clock() ;
	udb_init_capture() ;
//	udb_init_I2C() ;
//	udb_init_GPS() ;
//	udb_init_USART() ;
	udb_init_pwm() ;
#if (USE_OSD == 1)
	udb_init_osd() ;
#endif
	
	SRbits.IPL = 0 ;	// turn on all interrupt priorities
	
	return ;
}


void udb_run(void)
{
	//  nothing else to do... entirely interrupt driven
	while (1)
	{
		// pause cpu counting timer while not in an ISR
		indicate_loading_main ;
	}
	// Never returns
}


void udb_init_leds( void )
{
	return ;
}


// the compiler does not respect the resource used by the Microchip math
// library, so interrupts need to save and restore extended state, and
// reset CORCON if firing in the middle of a math lib call.
void udb_setDSPLibInUse(boolean inUse)
{
	needSaveExtendedState = inUse ;
	return ;
}


void udb_a2d_record_offsets(void)
{
	// almost ready to turn the control on, save the input offsets
	UDB_XACCEL.offset = UDB_XACCEL.value ;
	udb_xrate.offset = udb_xrate.value ;
	UDB_YACCEL.offset = UDB_YACCEL.value ;
	udb_yrate.offset = udb_yrate.value ;
	UDB_ZACCEL.offset = UDB_ZACCEL.value GRAVITY_SIGN ((int)(2*GRAVITY)) ;  // GRAVITY is measured in A-D/2 units
	udb_zrate.offset = udb_zrate.value ;									// The sign is for inverted boards
#ifdef VREF
	udb_vref.offset = udb_vref.value ;
#endif
	return ;
}


void udb_servo_record_trims(void)
{
	int i;
	for (i=0; i <= NUM_INPUTS; i++)
		udb_pwTrim[i] = udb_pwIn[i] ;
	
	return ;
}


// saturation logic to maintain pulse width within bounds
int udb_servo_pulsesat ( long pw )
{
	if ( pw > SERVOMAX ) pw = SERVOMAX ;
	if ( pw < SERVOMIN ) pw = SERVOMIN ;
	return (int)pw ;
}

/****************************************************************************/
// This is part of the servo and radio interface software
//
// ServoInterface source code
//	http://code.google.com/p/rc-servo-interface
//
// Copyright 2010 ServoInterface Team
// See the AUTHORS.TXT file for a list of authors of ServoInterface.
//
// ServoInterface is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ServoInterface is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License 
// along with ServoInterface.  If not, see <http://www.gnu.org/licenses/>.
//
// Many parts of ServoInterface use either modified or unmodified code
// from the MatrixPilot pilot project.
// The project also contains code for modifying MatrixPilot to operate
// with ServoInterface.
// For details, credits and licenses of MatrixPilot see the AUTHORS.TXT file.
// or see this website: http://code.google.com/p/gentlenav
/****************************************************************************/
