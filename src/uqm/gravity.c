//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

// JMS_GFX 2012: Merged the resolution Factor stuff from P6014.

#include "collide.h"
#include "races.h"
#include "units.h"
#include "libs/log.h"
#include "options.h"
#include "globdata.h"

//#define DEBUG_GRAVITY

BOOLEAN
CalculateGravity (ELEMENT *ElementPtr)
{
	BOOLEAN retval, HasGravity;
	HELEMENT hTestElement, hSuccElement;

	retval = FALSE;
	HasGravity = (BOOLEAN)(CollidingElement (ElementPtr)
			&& GRAVITY_MASS (ElementPtr->mass_points + 1));

	for (hTestElement = GetHeadElement ();
			hTestElement != 0; hTestElement = hSuccElement)
	{
		BOOLEAN TestHasGravity;
		ELEMENT *TestElementPtr;

		LockElement (hTestElement, &TestElementPtr);

		if (TestElementPtr != ElementPtr
				&& CollidingElement (TestElementPtr)
				&& (TestHasGravity =
				GRAVITY_MASS (TestElementPtr->mass_points + 1)) != HasGravity)
		{
			COUNT abs_dx, abs_dy;
			SIZE dx, dy;
			if (TestElementPtr->state_flags & PLAYER_SHIP) {
				STARSHIP *TestStarShipPtr;
				GetElementStarShip (TestElementPtr, &TestStarShipPtr);
				if (IS_RETREAT(TestStarShipPtr)) {
					STARSHIP *StarShipPtr;
					GetElementStarShip (ElementPtr, &StarShipPtr);
					if(StarShipPtr != NULL) {
						switch(StarShipPtr->SpeciesID) {
								case VUX_ID:
									// To make able to VUX to spawn near to retreating ship
									TestHasGravity = FALSE;
									break;
								default:
									break;
						}
					}
				}
			}

			if (!(ElementPtr->state_flags & PRE_PROCESS))
			{
				dx = ElementPtr->current.location.x
						- TestElementPtr->current.location.x;
				dy = ElementPtr->current.location.y
						- TestElementPtr->current.location.y;
			}
			else
			{
				dx = ElementPtr->next.location.x
						- TestElementPtr->next.location.x;
				dy = ElementPtr->next.location.y
						- TestElementPtr->next.location.y;
			}
#ifdef DEBUG_GRAVITY
			if (TestElementPtr->state_flags & PLAYER_SHIP)
			{
				log_add (log_Debug, "CalculateGravity:");
				log_add (log_Debug, "\tdx = %d, dy = %d", dx, dy);
			}
#endif /* DEBUG_GRAVITY */
			dx = WRAP_DELTA_X (dx);
			dy = WRAP_DELTA_Y (dy);
#ifdef DEBUG_GRAVITY
			if (TestElementPtr->state_flags & PLAYER_SHIP)
				log_add (log_Debug, "\twrap_dx = %d, wrap_dy = %d", dx, dy);
#endif /* DEBUG_GRAVITY */
			abs_dx = dx >= 0 ? dx : -dx;
			abs_dy = dy >= 0 ? dy : -dy;
			abs_dx = WORLD_TO_DISPLAY (abs_dx);
			abs_dy = WORLD_TO_DISPLAY (abs_dy);
#ifdef DEBUG_GRAVITY
			if (TestElementPtr->state_flags & PLAYER_SHIP)
				log_add (log_Debug, "\tdisplay_dx = %d, display_dy = %d",
						abs_dx, abs_dy);
#endif /* DEBUG_GRAVITY */
			if (abs_dx <= GRAVITY_THRESHOLD
					&& abs_dy <= GRAVITY_THRESHOLD)
			{
				DWORD dist_squared;

				dist_squared = (DWORD)(abs_dx * abs_dx)
						+ (DWORD)(abs_dy * abs_dy);
				if (dist_squared <= (DWORD)(GRAVITY_THRESHOLD
						* GRAVITY_THRESHOLD))
				{
#ifdef NEVER
					COUNT magnitude;

#define DIFUSE_GRAVITY (175 << RESOLUTION_FACTOR) // JMS_GFX: Because of the ifdef NEVER this is actually never run. Well, changed it for consistency
					dist_squared += (DWORD)abs_dx * (DIFUSE_GRAVITY << 1)
							+ (DWORD)abs_dy * (DIFUSE_GRAVITY << 1)
							+ ((DWORD)(DIFUSE_GRAVITY * DIFUSE_GRAVITY) << 1);
					if ((magnitude = (COUNT)((DWORD)(GRAVITY_THRESHOLD
							* GRAVITY_THRESHOLD) / dist_squared)) == 0)
						magnitude = 1;

#define MAX_MAGNITUDE (6 << RESOLUTION_FACTOR) // JMS_GFX: Because of the ifdef NEVER this is actually never run. Well, changed it for consistency
					else if (magnitude > MAX_MAGNITUDE)
						magnitude = MAX_MAGNITUDE;
					log_add (log_Debug, "magnitude = %u", magnitude);
#endif /* NEVER */

#ifdef DEBUG_GRAVITY
					if (TestElementPtr->state_flags & PLAYER_SHIP)
						log_add (log_Debug, "dist_squared = %lu", dist_squared);
#endif /* DEBUG_GRAVITY */
					if (TestHasGravity)
					{
						retval = TRUE;
						UnlockElement (hTestElement);
						break;
					}
					else
					{
						COUNT angle;

						angle = ARCTAN (dx, dy);
						DeltaVelocityComponents (&TestElementPtr->velocity,
								COSINE (angle, WORLD_TO_VELOCITY (1 << RESOLUTION_FACTOR)),
								SINE (angle, WORLD_TO_VELOCITY (1 << RESOLUTION_FACTOR))); // JMS_GFX
						if (TestElementPtr->state_flags & PLAYER_SHIP)
						{
							STARSHIP *StarShipPtr;

							GetElementStarShip (TestElementPtr, &StarShipPtr);
							StarShipPtr->cur_status_flags &= ~SHIP_AT_MAX_SPEED;
							StarShipPtr->cur_status_flags |= SHIP_IN_GRAVITY_WELL;
						}
					}
				}
			}
		}

		hSuccElement = GetSuccElement (TestElementPtr);
		UnlockElement (hTestElement);
	}

	return (retval);
}

BOOLEAN
Overlap (ELEMENT *ElementPtr)
//TimeSpaceMatterConflict (ELEMENT *ElementPtr) vanilla function
{
	HELEMENT hTestElement, hSuccElement;
	INTERSECT_CONTROL ElementControl;

	ElementControl.IntersectStamp.origin.x =
			WORLD_TO_DISPLAY (ElementPtr->current.location.x);
	ElementControl.IntersectStamp.origin.y =
			WORLD_TO_DISPLAY (ElementPtr->current.location.y);
	ElementControl.IntersectStamp.frame =
			 SetEquFrameIndex (ElementPtr->current.image.farray[0],
			 ElementPtr->current.image.frame);
	ElementControl.EndPoint = ElementControl.IntersectStamp.origin;
	for (hTestElement = GetHeadElement ();
			hTestElement != 0; hTestElement = hSuccElement)
	{
		ELEMENT *TestElementPtr;

		LockElement (hTestElement, &TestElementPtr);
		hSuccElement = GetSuccElement (TestElementPtr);
		if (TestElementPtr != ElementPtr
				&& (CollidingElement (TestElementPtr)
						/* ship in transition */
				|| (TestElementPtr->state_flags & PLAYER_SHIP)))
		{
			INTERSECT_CONTROL TestElementControl;

			TestElementControl.IntersectStamp.origin.x =
					WORLD_TO_DISPLAY (TestElementPtr->current.location.x);
			TestElementControl.IntersectStamp.origin.y =
					WORLD_TO_DISPLAY (TestElementPtr->current.location.y);
			TestElementControl.IntersectStamp.frame =
					 SetEquFrameIndex (TestElementPtr->current.image.farray[0],
					 TestElementPtr->current.image.frame);
			TestElementControl.EndPoint = TestElementControl.IntersectStamp.origin;
			if (DrawablesIntersect (&ElementControl, &TestElementControl, MAX_TIME_VALUE) && 
			    (ElementPtr->is_teleporting) && !(TestElementPtr->triggers_teleport_safety))
			    /* Let Arilou telefrag if it's not hitting a safe object. */
			{
				return FALSE;
			}
			if (DrawablesIntersect (&ElementControl,
					&TestElementControl, MAX_TIME_VALUE))
			{
				UnlockElement (hTestElement);

				break;
			}
		}
		UnlockElement (hTestElement);
	}

	return (hTestElement != 0 ? TRUE : FALSE);
}

