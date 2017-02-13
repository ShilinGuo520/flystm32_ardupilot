/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
  Flymaple port by Mike McCauley
  Monitor a PPM-SUM input pin, and decode the channels based on pulse widths
  Uses a timer to capture the time between negative transitions of the PPM-SUM pin
 */
#include <AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_FLYMAPLE

// Flymaple RCInput
// PPM input from a single pin

#include "RCInput.h"
#include "FlymapleWirish.h"

using namespace AP_HAL_FLYMAPLE_NS;

extern const AP_HAL::HAL& hal;


/* private variables to communicate with input capture isr */
volatile uint16_t FLYMAPLERCInput::_pulse_capt[FLYMAPLE_RC_INPUT_NUM_CHANNELS] = {0};  
volatile uint8_t  FLYMAPLERCInput::_valid_channels = 0;
volatile uint32_t FLYMAPLERCInput::_last_input_interrupt_time = 0; // Last time the input interrupt ran

// Pin 6 is connected to timer 1 channel 1
#define FLYMAPLE_RC_INPUT_PIN 6

// This is the rollover count of the timer
// each count is 0.5us, so 600000 = 30ms
// We cant reliably measure intervals that exceed this time.
#define FLYMAPLE_TIMER_RELOAD 60000

FLYMAPLERCInput::FLYMAPLERCInput()
{}

void FLYMAPLERCInput::init(void* machtnichts)
{
    /* initialize overrides */
    clear_overrides();
	/* Configuration uartC use for RCInput */
    hal.uartC->begin(115200, 128, 128); 
    hal.uartC->printf(" \n\rInit uartC in RCInput");
}

bool FLYMAPLERCInput::new_input() {
	uint8_t temp[2] = {0, 0};
	uint8_t ch = 0;
	uint8_t i = 0;
	if (hal.uartC->available()) {
		while(hal.uartC->available()) {
			temp[1] = temp[0];
			temp[0] = hal.uartC->read();
			if((temp[0] == 'H') && (temp[1] == 'C')) {
				temp[0] = temp[1] = 0;
				if(hal.uartC->available()) 
					ch = hal.uartC->read();
				if(hal.uartC->available())
					temp[0] = hal.uartC->read();
				if(hal.uartC->available())
					temp[1] = hal.uartC->read();
				if((temp[0] != 0) || (temp[1] != 0)) {
					_pulse_capt[ch - 1] = (temp[0] << 8) | temp[1];
					temp[0] = temp[1] = 0;
				}
			}
		}
		_valid_channels = 8;
	} else {
		_valid_channels = 0;
	}
	return _valid_channels;
}

uint8_t FLYMAPLERCInput::num_channels() {
    return _valid_channels;
}

/* constrain captured pulse to be between min and max pulsewidth. */
static inline uint16_t constrain_pulse(uint16_t p) {
    if (p > RC_INPUT_MAX_PULSEWIDTH) return RC_INPUT_MAX_PULSEWIDTH;
    if (p < RC_INPUT_MIN_PULSEWIDTH) return RC_INPUT_MIN_PULSEWIDTH;
    return p;
}

uint16_t FLYMAPLERCInput::read(uint8_t ch) {
    timer_dev *tdev = PIN_MAP[FLYMAPLE_RC_INPUT_PIN].timer_device;
    uint8 timer_channel = PIN_MAP[FLYMAPLE_RC_INPUT_PIN].timer_channel;

    /* constrain ch */
    if (ch >= FLYMAPLE_RC_INPUT_NUM_CHANNELS) 
	return 0;
    /* grab channel from isr's memory in critical section*/
    timer_disable_irq(tdev, timer_channel);
    uint16_t capt = _pulse_capt[ch];
    timer_enable_irq(tdev, timer_channel);
    /* scale _pulse_capt from 0.5us units to 1us units. */
    uint16_t pulse = constrain_pulse(capt >> 1);
    /* Check for override */
    uint16_t over = _override[ch];
    return (over == 0) ? pulse : over;
}

uint8_t FLYMAPLERCInput::read(uint16_t* periods, uint8_t len) {
    timer_dev *tdev = PIN_MAP[FLYMAPLE_RC_INPUT_PIN].timer_device;
    uint8 timer_channel = PIN_MAP[FLYMAPLE_RC_INPUT_PIN].timer_channel;

    /* constrain len */
    if (len > FLYMAPLE_RC_INPUT_NUM_CHANNELS) 
	len = FLYMAPLE_RC_INPUT_NUM_CHANNELS;
    /* grab channels from isr's memory in critical section */
    timer_disable_irq(tdev, timer_channel);
    for (uint8_t i = 0; i < len; i++) {
        periods[i] = _pulse_capt[i];
    }
    timer_enable_irq(tdev, timer_channel);
    /* Outside of critical section, do the math (in place) to scale and
     * constrain the pulse. */
    for (uint8_t i = 0; i < len; i++) {
        /* scale _pulse_capt from 0.5us units to 1us units. */
        periods[i] = constrain_pulse(periods[i] >> 1);
        /* check for override */
        if (_override[i] != 0) {
            periods[i] = _override[i];
        }
    }
    return _valid_channels;
}

bool FLYMAPLERCInput::set_overrides(int16_t *overrides, uint8_t len) {
    bool res = false;
    for (uint8_t i = 0; i < len; i++) {
        res |= set_override(i, overrides[i]);
    }
    return res;
}

bool FLYMAPLERCInput::set_override(uint8_t channel, int16_t override) {
    if (override < 0) return false; /* -1: no change. */
    if (channel < FLYMAPLE_RC_INPUT_NUM_CHANNELS) {
        _override[channel] = override;
        if (override != 0) {
            _valid_channels = 1;
            return true;
        }
    }
    return false;
}

void FLYMAPLERCInput::clear_overrides()
{
    for (uint8_t i = 0; i < FLYMAPLE_RC_INPUT_NUM_CHANNELS; i++) {
        _override[i] = 0;
    }
}

#endif
