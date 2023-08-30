/* GamepadCursor.h
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/
#ifndef GAMEPAD_CURSOR_INCLUDED
#define GAMEPAD_CURSOR_INCLUDED


#include "Point.h"

// Track and draw the gamepad cursor
class GamepadCursor
{
public:
	static void SetPosition(const Point& pos);
	static void SetEnabled(bool enabled);
	static const Point& Position() { return s_position; }
	static bool Enabled() { return s_enabled; }

	static void Draw();

private:
	static Point s_position;
	static bool s_enabled;
};


#endif