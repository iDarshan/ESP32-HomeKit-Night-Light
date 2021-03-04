/*
HomeKit supported Night Light with ESP32 board.

Copyright (C) 2021  Darshan Prajapati

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _NIGHTLIGHT_H_
#define _NIGHTLIGHT_H_

/**
 * @brief initialize the night_light lowlevel module
 *
 * @param none
 *
 * @return none
 */
void night_light_init(void);

/**
 * @brief deinitialize the night_light's lowlevel module
 *
 * @param none
 *
 * @return none
 */
void night_light_deinit(void);

/**
 * @brief turn on/off the lowlevel night_light
 *
 * @param value The "On" value
 *
 * @return none
 */
int night_light_set_on(bool value);

/**
 * @brief set the saturation of the lowlevel night_light
 *
 * @param value The Saturation value
 *
 * @return 
 *     - 0 : OK
 *     - others : fail
 */
int night_light_set_saturation(float value);

/**
 * @brief set the hue of the lowlevel night_light
 *
 * @param value The Hue value
 *
 * @return 
 *     - 0 : OK
 *     - others : fail
 */
int night_light_set_hue(float value);

/**
 * @brief set the brightness of the lowlevel night_light
 *
 * @param value The Brightness value
 *
 * @return 
 *     - 0 : OK
 *     - others : fail
 */
int night_light_set_brightness(int value);

#endif /* _NIGHTLIGHT_H_ */
