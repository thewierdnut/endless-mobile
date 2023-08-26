/* GamepadPanel.cpp
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "GamepadPanel.h"
#include "FillShader.h"
#include "GamePad.h"
#include "RingShader.h"
#include "Screen.h"
#include "ShipInfoPanel.h"

#include "SpriteSet.h"
#include "text/alignment.hpp"
#include "CategoryList.h"
#include "CategoryTypes.h"
#include "Command.h"
#include "Dialog.h"
#include "text/DisplayText.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "LineShader.h"
#include "LogbookPanel.h"
#include "Messages.h"
#include "MissionPanel.h"
#include "OutlineShader.h"
#include "PlayerInfo.h"
#include "PlayerInfoPanel.h"
#include "Rectangle.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteShader.h"
#include "text/Table.h"
#include "text/truncate.hpp"
#include "UI.h"

#include <SDL_gamecontroller.h>
#include <algorithm>
#include <ctime>

namespace
{
	const std::vector<std::string> NO_CONTROLLERS{"No Controllers Connected"};

	const std::vector<SDL_GameControllerButton> CONFIGURABLE_BUTTONS = {
		SDL_CONTROLLER_BUTTON_A,
		SDL_CONTROLLER_BUTTON_B,
		SDL_CONTROLLER_BUTTON_X,
		SDL_CONTROLLER_BUTTON_Y,
		SDL_CONTROLLER_BUTTON_BACK,
		SDL_CONTROLLER_BUTTON_GUIDE,
		SDL_CONTROLLER_BUTTON_START,
		SDL_CONTROLLER_BUTTON_LEFTSTICK,
		SDL_CONTROLLER_BUTTON_RIGHTSTICK,
		SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
		SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
		SDL_CONTROLLER_BUTTON_DPAD_UP,
		SDL_CONTROLLER_BUTTON_DPAD_DOWN,
		SDL_CONTROLLER_BUTTON_DPAD_LEFT,
		SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
	};
	const std::vector<SDL_GameControllerAxis> CONFIGURABLE_AXES = {
		SDL_CONTROLLER_AXIS_LEFTX,  // negative direction
		SDL_CONTROLLER_AXIS_LEFTX,  // positive
		SDL_CONTROLLER_AXIS_LEFTY,  // negative
		SDL_CONTROLLER_AXIS_LEFTY,  // positive
		SDL_CONTROLLER_AXIS_RIGHTX, // negative
		SDL_CONTROLLER_AXIS_RIGHTX, // positive
		SDL_CONTROLLER_AXIS_RIGHTY, // negative
		SDL_CONTROLLER_AXIS_RIGHTY, // positive
		SDL_CONTROLLER_AXIS_TRIGGERLEFT,
		SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
	};
}

GamepadPanel::GamepadPanel():
	ui(GameData::Interfaces().Get("gamepad panel"))
{
	gamepadList.SetPadding(0);
	gamepadList.ShowDropIcon(true);
	gamepadList.SetOptions(NO_CONTROLLERS);
	gamepadList.SetCallback([this](int idx, const std::string&) { GamePad::SetControllerIdx(idx); reloadGamepad = true; });
	gamepadList.SetBgColor(*GameData::Colors().Get("shop info panel background"));
}

void GamepadPanel::Step()
{
	if(reloadGamepad)
	{
		auto controller_list = GamePad::GetControllerList();
		if(controller_list.empty())
		{
			gamepadList.SetOptions(NO_CONTROLLERS);
			gamepadList.SetSelectedIndex(0);
		}
		else
		{
			gamepadList.SetOptions(controller_list);
			gamepadList.SetSelectedIndex(GamePad::CurrentControllerIdx());
		}

		reloadGamepad = false;
	}

	if(remapIdx != -1)
	{
		// we are in remapping mode. Check if a button has been pressed.
		std::string input = GamePad::GetNextJoystickInput();
		if(!input.empty())
		{
			if(static_cast<size_t>(remapIdx) < CONFIGURABLE_BUTTONS.size())
			{
				const char* buttonName = SDL_GameControllerGetStringForButton(CONFIGURABLE_BUTTONS[remapIdx]);
				GamePad::SetControllerButtonMapping(buttonName, input);
			}
			else if(static_cast<size_t>(remapIdx) < CONFIGURABLE_BUTTONS.size() + CONFIGURABLE_AXES.size())
			{
				int axisIdx = remapIdx - CONFIGURABLE_BUTTONS.size();
				std::string axisName = SDL_GameControllerGetStringForAxis(CONFIGURABLE_AXES[axisIdx]);

				// we map left and right directions separately, so check if this
				// odd or even. Triggers are an exception.
				if(axisName.find("trigger") == std::string::npos)
				{
					if((axisIdx & 1) == 0)
						axisName = '-' + axisName; // even ones are negative
					else
						axisName = '+' + axisName; // odd ones are positive

				}
				GamePad::SetControllerButtonMapping(axisName, input);
			}
			++remapIdx;
			if(static_cast<size_t>(remapIdx) >= CONFIGURABLE_BUTTONS.size() + CONFIGURABLE_AXES.size())
			{
				// we are done.
				remapIdx = -1;
				GamePad::SaveMapping();
				mapping_saved = true;
			}
			else
			{
				// request the next input
				GamePad::CaptureNextJoystickInput();
			}
			UpdateUserMessage();
		}
	}
}



void GamepadPanel::Draw()
{
	// Dim everything behind this panel.
	const Color &back = *GameData::Colors().Get("dialog backdrop");
	FillShader::Fill(Point(), Point(Screen::Width(), Screen::Height()), back);
	
	const GamePad::Buttons &buttons = GamePad::Held();
	const GamePad::Axes &axes = GamePad::Positions();

	Information info;
	info.SetString("status", userMessage);
	if(remapIdx == -1) // not remapping. Just display the pressed buttons
	{
		if(buttons[SDL_CONTROLLER_BUTTON_B])             info.SetBar("B Button", 1);
		if(buttons[SDL_CONTROLLER_BUTTON_A])             info.SetBar("A Button", 1);
		if(buttons[SDL_CONTROLLER_BUTTON_Y])             info.SetBar("Y Button", 1);
		if(buttons[SDL_CONTROLLER_BUTTON_X])             info.SetBar("X Button", 1);
		if(buttons[SDL_CONTROLLER_BUTTON_GUIDE])         info.SetBar("Guide Button", 1);
		if(buttons[SDL_CONTROLLER_BUTTON_START])         info.SetBar("Start Button", 1);
		if(buttons[SDL_CONTROLLER_BUTTON_BACK])          info.SetBar("Back Button", 1);
		if(buttons[SDL_CONTROLLER_BUTTON_LEFTSHOULDER])  info.SetCondition("Left Shoulder Button");
		if(buttons[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER]) info.SetCondition("Right Shoulder Button");
		if(axes[SDL_CONTROLLER_AXIS_TRIGGERLEFT] > 16000) info.SetCondition("Left Trigger Button");
		if(axes[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] > 16000) info.SetCondition("Right Trigger Button");
		if(buttons[SDL_CONTROLLER_BUTTON_LEFTSTICK])     info.SetBar("Left Stick Button", 1);
		if(buttons[SDL_CONTROLLER_BUTTON_RIGHTSTICK])    info.SetBar("Right Stick Button", 1);
		if(buttons[SDL_CONTROLLER_BUTTON_DPAD_LEFT])     info.SetCondition("Left Dpad Button");
		if(buttons[SDL_CONTROLLER_BUTTON_DPAD_RIGHT])    info.SetCondition("Right Dpad Button");
		if(buttons[SDL_CONTROLLER_BUTTON_DPAD_UP])       info.SetCondition("Up Dpad Button");
		if(buttons[SDL_CONTROLLER_BUTTON_DPAD_DOWN])     info.SetCondition("Down Dpad Button");
		
		if(axes[SDL_CONTROLLER_AXIS_LEFTX] < -16000)     info.SetCondition("Left Joystick Left");
		if(axes[SDL_CONTROLLER_AXIS_LEFTX] > 16000)      info.SetCondition("Left Joystick Right");
		if(axes[SDL_CONTROLLER_AXIS_LEFTY] < -16000)     info.SetCondition("Left Joystick Up");
		if(axes[SDL_CONTROLLER_AXIS_LEFTY] > 16000)      info.SetCondition("Left Joystick Down");
		if(axes[SDL_CONTROLLER_AXIS_RIGHTX] < -16000)    info.SetCondition("Right Joystick Left");
		if(axes[SDL_CONTROLLER_AXIS_RIGHTX] > 16000)     info.SetCondition("Right Joystick Right");
		if(axes[SDL_CONTROLLER_AXIS_RIGHTY] < -16000)    info.SetCondition("Right Joystick Up");
		if(axes[SDL_CONTROLLER_AXIS_RIGHTY] > 16000)     info.SetCondition("Right Joystick Down");
	}
	else
	{
		// TODO: combine these two cases somehow?
		if(static_cast<size_t>(remapIdx) < CONFIGURABLE_BUTTONS.size())
		{
			SDL_GameControllerButton button = CONFIGURABLE_BUTTONS[remapIdx];
			if(button == SDL_CONTROLLER_BUTTON_B)             info.SetBar("B Button", 1);
			if(button == SDL_CONTROLLER_BUTTON_A)             info.SetBar("A Button", 1);
			if(button == SDL_CONTROLLER_BUTTON_Y)             info.SetBar("Y Button", 1);
			if(button == SDL_CONTROLLER_BUTTON_X)             info.SetBar("X Button", 1);
			if(button == SDL_CONTROLLER_BUTTON_GUIDE)         info.SetBar("Guide Button", 1);
			if(button == SDL_CONTROLLER_BUTTON_START)         info.SetBar("Start Button", 1);
			if(button == SDL_CONTROLLER_BUTTON_BACK)          info.SetBar("Back Button", 1);
			if(button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)  info.SetCondition("Left Shoulder Button");
			if(button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) info.SetCondition("Right Shoulder Button");
			if(button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)     info.SetCondition("Left Dpad Button");
			if(button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)    info.SetCondition("Right Dpad Button");
			if(button == SDL_CONTROLLER_BUTTON_DPAD_UP)       info.SetCondition("Up Dpad Button");
			if(button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)     info.SetCondition("Down Dpad Button");

			if(button == SDL_CONTROLLER_BUTTON_LEFTSTICK)     info.SetCondition("Left Stick Button");
			if(button == SDL_CONTROLLER_BUTTON_RIGHTSTICK)    info.SetCondition("Right Stick Button");
		}
		else if(static_cast<size_t>(remapIdx) < CONFIGURABLE_BUTTONS.size() + CONFIGURABLE_AXES.size())
		{
			int axisIdx = remapIdx - CONFIGURABLE_BUTTONS.size();
			SDL_GameControllerAxis axis = CONFIGURABLE_AXES[axisIdx];
			bool negative = (axisIdx & 1) == 0;

			if(axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)  info.SetCondition("Left Trigger Button");
			if(axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) info.SetCondition("Right Trigger Button");
			if(axis == SDL_CONTROLLER_AXIS_LEFTX)        info.SetCondition(negative ? "Left Joystick Left" : "Left Joystick Right");
			if(axis == SDL_CONTROLLER_AXIS_LEFTY)        info.SetCondition(negative ? "Left Joystick Up": "Left Joystick Down");
			if(axis == SDL_CONTROLLER_AXIS_RIGHTX)       info.SetCondition(negative ? "Right Joystick Left": "Right Joystick Right");
			if(axis == SDL_CONTROLLER_AXIS_RIGHTY)       info.SetCondition(negative ? "Right Joystick Up": "Right Joystick Down");
		}
	}

	if(gamepadList.GetSelected() != NO_CONTROLLERS.front())
		info.SetCondition("has controller");

	ui->Draw(info, this);

	const Color* color = GameData::Colors().Get("shields");

	// don't draw the joystick positions in remap mode
	if(remapIdx == -1)
	{
		Point leftJoystick = GamePad::LeftStick();
		if(leftJoystick)
		{
			auto rect = ui->GetBox("Left Joystick");
			Point position = rect.Center() + leftJoystick * (1.0 / 65536.0) * rect.Width()/2;
			
			RingShader::Draw(position, 25, 0, *color);
		}

		Point rightJoystick = GamePad::RightStick();
		if(rightJoystick)
		{
			auto rect = ui->GetBox("Right Joystick");
			Point position = rect.Center() + rightJoystick * (1.0 / 65536.0) * rect.Width()/2;
			RingShader::Draw(position, 25, 0, *color);
		}
	}

	auto buttonListRect = ui->GetBox("Button List");
	
	Table t;
	t.SetHighlight(0, buttonListRect.Width());
	t.AddColumn(0, {0, Alignment::LEFT});
	t.AddColumn(buttonListRect.Width(), {0, Alignment::RIGHT});
	t.DrawAt(buttonListRect.TopLeft());

	for (auto& s: GamePad::GetCurrentSdlMappings())
	{
		auto axis = SDL_GameControllerGetAxisFromString(s.first.c_str());
		auto button = SDL_GameControllerGetButtonFromString(s.first.c_str());
		if((button != SDL_CONTROLLER_BUTTON_INVALID && buttons[button]) ||
		   (axis != SDL_CONTROLLER_AXIS_INVALID && (axes[axis] > 16000 || axes[axis] < -16000)))
		{
			t.DrawHighlight(*color);
		}

		// Drawing text here takes 300k ticks
		t.Draw(s.first);
		t.Draw(s.second);
	}

	auto gamepadListRect = ui->GetBox("Gamepad Dropdown");
	gamepadList.SetPosition(gamepadListRect);
	gamepadList.Draw(this);
}



bool GamepadPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool /* isNewPress */)
{
	bool control = (mod & (KMOD_CTRL | KMOD_GUI));
	//bool shift = (mod & KMOD_SHIFT);
	if(key == 'b' || key == SDLK_ESCAPE || key == SDLK_AC_BACK  || (key == 'w' && control))
	{
		if(remapIdx == -1)
			GetUI()->Pop(this); 	// quit the dialog
		else
		{
			// Skip this mapping
			++remapIdx;
			if(static_cast<size_t>(remapIdx) < CONFIGURABLE_BUTTONS.size() + CONFIGURABLE_AXES.size())
				GamePad::CaptureNextJoystickInput();
			else
			{
				remapIdx = -1;
				GamePad::SaveMapping();
				mapping_saved = true;
				UpdateUserMessage();
			}
		}
	}
	else if(key == 'r')
	{
		remapIdx = 0; // Enter remap mode.
		UpdateUserMessage();
		GamePad::ClearMappings();
		GamePad::CaptureNextJoystickInput();
	}
	else
		return false;

	return true;
}



bool GamepadPanel::ControllersChanged()
{
	reloadGamepad = true;
	return true;
}



void GamepadPanel::UpdateUserMessage()
{
	if(remapIdx == -1)
	{
		if(mapping_saved)
		{
			userMessage = "Mapping Saved.";
		}
	}
	else
	{
		if(static_cast<size_t>(remapIdx) < CONFIGURABLE_BUTTONS.size())
		{
			const char* buttonName = SDL_GameControllerGetStringForButton(CONFIGURABLE_BUTTONS[remapIdx]);
			userMessage = buttonName ? buttonName : "";
			userMessage += ": Waiting for input (press escape or back to skip)";
		}
		else if(static_cast<size_t>(remapIdx) < CONFIGURABLE_BUTTONS.size() + CONFIGURABLE_AXES.size())
		{
			int axisIdx = remapIdx - CONFIGURABLE_BUTTONS.size();
			const char* axisName = SDL_GameControllerGetStringForAxis(CONFIGURABLE_AXES[axisIdx]);
			userMessage = axisName ? axisName : "";
			userMessage += ": Waiting for input (press escape or back to skip)";
		}
	}
}