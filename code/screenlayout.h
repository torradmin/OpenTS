/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "point.h"
#include "rect.h"

/*
 * The largest interface magnification the game will use. Beyond this the sidebar takes more
 * of the screen than the world it belongs to.
 */
int const UI_SCALE_MAX = 4;

/// <summary>
/// How a screen of a given size is divided between the world and the interface.
/// </summary>
struct ScreenLayout
{
	/*
	 * The whole visible surface, at the frame's own resolution.
	 */
	Rect Hidden;

	/*
	 * The tactical view, in screen coordinates.
	 */
	Rect Tactical;

	/*
	 * The composite and tile surfaces, which are drawn at the tactical view's size but keep
	 * their own coordinate origin.
	 */
	Rect Composite;
	Rect Tile;

	/*
	 * The interface column, in the sidebar surface's own coordinates. Blit_Sidebar magnifies
	 * it by the interface scale on its way to the screen.
	 */
	Rect Sidebar;
};

ScreenLayout Compute_Screen_Layout(Rect const & visible);

/// <summary>
/// How many screen pixels the interface is drawn at for each pixel of its own artwork.
/// </summary>
/// <param name="framewidth">The width of the frame the interface has to fit into.</param>
/// <param name="frameheight">The height of the frame the interface has to fit into.</param>
/// <returns>Returns with the magnification, between one and UI_SCALE_MAX.</returns>
/// <remarks>A configured scale of zero asks for one that follows the frame, and one the frame
/// is too small to carry is stepped back down to one it can.</remarks>
int UI_Scale(int framewidth, int frameheight);
int UI_Scale(void);

/// <summary>
/// Converts a rectangle of the sidebar surface into the screen rectangle it is magnified into.
/// </summary>
/// <param name="rect">The rectangle in the sidebar surface's own coordinates.</param>
/// <returns>Returns with the same area expressed in screen pixels.</returns>
Rect Sidebar_To_Screen(Rect const & rect);

/// <summary>
/// Converts a screen point into the sidebar surface pixel drawn beneath it.
/// </summary>
/// <param name="point">The point in screen pixels.</param>
/// <returns>Returns with the point in the sidebar surface's own coordinates.</returns>
Point2D Screen_To_Sidebar(Point2D const & point);
