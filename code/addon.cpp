/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "addon.h"

#include "_rules.h"
#include "ccfile.h"
#include "ccini.h"
#include "data.h"
#include "init.h"
#include "language/language.h"
#include "ownrdraw.h"
#include "sun.h"

BOOL CALLBACK Select_Game_Type_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

int AvailableAddOns = 1 << ADDON_BASE_GAME;
int ActiveAddOns = 1 << ADDON_BASE_GAME;
AddonType RequiredAddon = ADDON_BASE_GAME;

/// <summary>
/// Steps an addon type on to the value after it.
/// </summary>
/// <returns>Returns with the incremented value.</returns>
AddonType operator++(AddonType & val)
{
	val = AddonType(int(val) + 1);
	return(val);
}


/// <summary>
/// Steps an addon type back to the value before it.
/// </summary>
/// <returns>Returns with the decremented value.</returns>
AddonType operator--(AddonType & val)
{
	val = AddonType(int(val) - 1);
	return(val);
}


/// <summary>
/// Asks the player which game they wish to play.
/// If [Options] Addon= is present in SUN.INI, that value is used and the dialog is skipped.
/// Otherwise this routine puts up the game type dialog whenever an expansion has been
/// detected, and enables whichever addon the player settles on. With nothing but the base
/// game present there is no choice to make, so the dialog never appears. A choice made from
/// the dialog is written back to the same key when it was already present, so an opted-in
/// player gets it kept in step with whichever addon was picked most recently.
/// </summary>
/// <param name="type">The addon that the player chose to play.</param>
/// <returns>bool; Should the game carry on? Returns false if the player backed out.</returns>
bool Select_Game_Type_Dialog(AddonType &type)
{
	int retval;

	type = ADDON_BASE_GAME;

	if (ConfigINI.Is_Present("Options", "Addon")) {
		if (ConfigINI.Get_Int("Options", "Addon", ADDON_BASE_GAME) == ADDON_FIRESTORM
			&& Addon_Installed(ADDON_FIRESTORM)) {
			Enable_Addon(ADDON_FIRESTORM);
			type = ADDON_FIRESTORM;
		}

		Set_Required_Addon(type);
		return(true);
	}

	if (Addon_Installed(ADDON_ANY)) {
		HWND dialog = OwnerDraw::Begin_Dialog(IDD_SELECT_GAME_TYPE, Select_Game_Type_Dialog_Proc);
		if (dialog != 0) {

			SetWindowLong(dialog, DWL_USER, (LONG)&retval);
			OwnerDraw::Display_Dialog(dialog);

			retval = -1;
			while (retval == -1) {
				if (OwnerDraw::Dialog_Message_Handler() == true) {
					break;
				}

				Title_Screen_Restore(false);
			}

			ShowWindow(dialog, SW_HIDE);
			UpdateWindow(MainWindow);
			OwnerDraw::End_Dialog(dialog);
			ActiveAddOns = 1 << ADDON_BASE_GAME;

			switch (retval) {
				default:
					type = ADDON_BASE_GAME;
					break;

				case IDC_GAMETYPE_FIRESTORM:
					Enable_Addon(ADDON_FIRESTORM);
					type = ADDON_FIRESTORM;
					break;

				case IDCANCEL:
					return(false);
			}

			Save_Addon_Type(type);
		}

		Set_Required_Addon(type);
		return(true);
	}

	return(true);
}


/// <summary>
/// Handles the messages for the game type selection dialog.
/// This routine stashes the control that the player pressed into the caller's result
/// variable, which is what lets the dialog loop know it can stop.
/// </summary>
BOOL CALLBACK Select_Game_Type_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	int * retval;

	int rc = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);

	if (rc == 0) {
		switch (message) {
			case WM_COMMAND:
				retval = (int *)GetWindowLong(window, DWL_USER);
				*retval = LOWORD(wparam);
				break;
		}
		rc = 0;
	}

	return(rc);
}


/// <summary>
/// Rebuilds the installed and active addon sets from the expansion rules files present.
/// </summary>
void Detect_Addons(void)
{
	AvailableAddOns = (1 << ADDON_BASE_GAME);
	ActiveAddOns = (1 << ADDON_BASE_GAME);

	if (CCFileClass("FIRESTRM.INI").Is_Available() == true) {
		AvailableAddOns |= (1 << ADDON_FIRESTORM);
	}
}


/// <summary>
/// Is the specified addon installed on this machine?
/// Pass ADDON_ANY to ask whether any expansion at all was found.
/// </summary>
/// <returns>bool; Is the addon present?</returns>
bool Addon_Installed(AddonType addon)
{
	if (addon == ADDON_ANY) {
		return((AvailableAddOns & ~(1 << ADDON_BASE_GAME)) != false);
	}

	return((AvailableAddOns & (1 << addon)) != false);
}


/// <summary>
/// Is the specified addon switched on?
/// An addon must be both installed and enabled to count. Pass ADDON_ANY to ask whether any
/// expansion at all is currently in force.
/// </summary>
/// <returns>bool; Is the addon active?</returns>
bool Addon_Enabled(AddonType addon)
{
	if (addon == ADDON_ANY) {
		return((ActiveAddOns & AvailableAddOns & ~(1 << ADDON_BASE_GAME)) != false);
	}
	return((ActiveAddOns & AvailableAddOns & (1 << addon)) != false);
}


/// <summary>
/// Switches on an installed addon.
/// Only an addon that was actually detected can be enabled, so asking for one that is not
/// present is harmless. Pass ADDON_ANY to switch on everything that is installed.
/// </summary>
void Enable_Addon(AddonType addon)
{
	if (addon == ADDON_ANY) {
		ActiveAddOns = AvailableAddOns;
	} else if (addon > ADDON_BASE_GAME) {
		ActiveAddOns |= AvailableAddOns & (1 << addon);
	}
}


/// <summary>
/// Switches an addon back off.
/// Pass ADDON_ANY to drop all the way back to the base game.
/// </summary>
void Disable_Addon(AddonType addon)
{
	if (addon == ADDON_ANY) {
		ActiveAddOns = (1 << ADDON_BASE_GAME);
	} else if (addon > ADDON_BASE_GAME) {
		ActiveAddOns &= ~(1 << addon);
	}
}


/// <summary>
/// Is the specified addon the one the current game demands?
/// </summary>
/// <returns>bool; Is this the required addon?</returns>
bool Is_Required_Addon(AddonType addon)
{
	return(addon == RequiredAddon);
}


/// <summary>
/// Fetches the addon that the current game demands.
/// </summary>
/// <returns>Returns with the required addon.</returns>
AddonType Get_Required_Addon(void)
{
	return(RequiredAddon);
}


/// <summary>
/// Sets the addon that the current game demands.
/// This routine is called when the player picks a game type from the menus, and again when
/// a scenario or a saved game states which addon its rules were built for.
/// </summary>
void Set_Required_Addon(AddonType addon)
{
	RequiredAddon = addon;
}


/// <summary>
/// Keeps SUN.INI's [Options] Addon= key in step with the given addon, if that key is
/// already present. A player who never added the key is left alone; one who did gets it
/// updated so it keeps naming whichever addon was picked most recently.
/// </summary>
void Save_Addon_Type(AddonType addon)
{
	if (!ConfigINI.Is_Present("Options", "Addon")) {
		return;
	}

	CDFileClass file(CONFIG_FILE_NAME);

	ConfigINI.Put_Int("Options", "Addon", addon);
	ConfigINI.Save(file, false);
}


/// <summary>
/// Fetches the display name of an addon.
/// </summary>
/// <returns>Returns with the localized title text for the addon specified.</returns>
const char * Get_Addon_Title(AddonType addon)
{
	static int _id[ADDON_COUNT] = {
		TXT_SHORT_TITLE,
		TXT_EXPANSION_TITLE,
	};
	return(Fetch_String(_id[addon]));
}
