/*
 * CGuiHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CGuiHandler.h"
#include "../lib/CondSh.h"

#include "CIntObject.h"
#include "CursorHandler.h"
#include "ShortcutHandler.h"

#include "../CGameInfo.h"
#include "../render/Colors.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../CMT.h"
#include "../CPlayerInterface.h"
#include "../battle/BattleInterface.h"

#include "../../lib/CThreadHelper.h"
#include "../../lib/CConfigHandler.h"

#include <SDL_render.h>
#include <SDL_timer.h>
#include <SDL_events.h>
#include <SDL_keycode.h>

#ifdef VCMI_APPLE
#include <dispatch/dispatch.h>
#endif

#ifdef VCMI_AURORAOS
#include <SDL_video.h>
#include <SDL_syswm.h>
#include <wayland-client-protocol.h>
#endif

#ifdef VCMI_IOS
#include "ios/utils.h"
#endif

extern std::queue<SDL_Event> SDLEventsQueue;
extern boost::mutex eventsM;

boost::thread_specific_ptr<bool> inGuiThread;

SObjectConstruction::SObjectConstruction(CIntObject *obj)
:myObj(obj)
{
	GH.createdObj.push_front(obj);
	GH.captureChildren = true;
}

SObjectConstruction::~SObjectConstruction()
{
	assert(GH.createdObj.size());
	assert(GH.createdObj.front() == myObj);
	GH.createdObj.pop_front();
	GH.captureChildren = GH.createdObj.size();
}

SSetCaptureState::SSetCaptureState(bool allow, ui8 actions)
{
	previousCapture = GH.captureChildren;
	GH.captureChildren = false;
	prevActions = GH.defActionsDef;
	GH.defActionsDef = actions;
}

SSetCaptureState::~SSetCaptureState()
{
	GH.captureChildren = previousCapture;
	GH.defActionsDef = prevActions;
}

static inline void
processList(const ui16 mask, const ui16 flag, std::list<CIntObject*> *lst, std::function<void (std::list<CIntObject*> *)> cb)
{
	if (mask & flag)
		cb(lst);
}

void CGuiHandler::processLists(const ui16 activityFlag, std::function<void (std::list<CIntObject*> *)> cb)
{
	processList(CIntObject::LCLICK,activityFlag,&lclickable,cb);
	processList(CIntObject::RCLICK,activityFlag,&rclickable,cb);
	processList(CIntObject::MCLICK,activityFlag,&mclickable,cb);
	processList(CIntObject::HOVER,activityFlag,&hoverable,cb);
	processList(CIntObject::MOVE,activityFlag,&motioninterested,cb);
	processList(CIntObject::KEYBOARD,activityFlag,&keyinterested,cb);
	processList(CIntObject::TIME,activityFlag,&timeinterested,cb);
	processList(CIntObject::WHEEL,activityFlag,&wheelInterested,cb);
	processList(CIntObject::DOUBLECLICK,activityFlag,&doubleClickInterested,cb);
	processList(CIntObject::TEXTINPUT,activityFlag,&textInterested,cb);
}

void CGuiHandler::init()
{
	shortcutsHandlerInstance = std::make_unique<ShortcutHandler>();
	mainFPSmng = new CFramerateManager();
	mainFPSmng->init(settings["video"]["targetfps"].Integer());
	isPointerRelativeMode = settings["general"]["userRelativePointer"].Bool();
	pointerSpeedMultiplier = settings["general"]["relativePointerSpeedMultiplier"].Float();
}

void CGuiHandler::handleElementActivate(CIntObject * elem, ui16 activityFlag)
{
	processLists(activityFlag,[&](std::list<CIntObject*> * lst){
		lst->push_front(elem);
	});
	elem->active_m |= activityFlag;
}

void CGuiHandler::handleElementDeActivate(CIntObject * elem, ui16 activityFlag)
{
	processLists(activityFlag,[&](std::list<CIntObject*> * lst){
		auto hlp = std::find(lst->begin(),lst->end(),elem);
		assert(hlp != lst->end());
		lst->erase(hlp);
	});
	elem->active_m &= ~activityFlag;
}

void CGuiHandler::popInt(std::shared_ptr<IShowActivatable> top)
{
	assert(listInt.front() == top);
	top->deactivate();
	disposed.push_back(top);
	listInt.pop_front();
	objsToBlit -= top;
	if(!listInt.empty())
		listInt.front()->activate();
	totalRedraw();
}

void CGuiHandler::pushInt(std::shared_ptr<IShowActivatable> newInt)
{
	assert(newInt);
	assert(!vstd::contains(listInt, newInt)); // do not add same object twice

	//a new interface will be present, we'll need to use buffer surface (unless it's advmapint that will alter screenBuf on activate anyway)
	screenBuf = screen2;

	if(!listInt.empty())
		listInt.front()->deactivate();
	listInt.push_front(newInt);
	CCS->curh->set(Cursor::Map::POINTER);
	newInt->activate();
	objsToBlit.push_back(newInt);
	totalRedraw();
}

void CGuiHandler::popInts(int howMany)
{
	if(!howMany) return; //senseless but who knows...

	assert(listInt.size() >= howMany);
	listInt.front()->deactivate();
	for(int i=0; i < howMany; i++)
	{
		objsToBlit -= listInt.front();
		disposed.push_back(listInt.front());
		listInt.pop_front();
	}

	if(!listInt.empty())
	{
		listInt.front()->activate();
		totalRedraw();
	}
	fakeMouseMove();
}

std::shared_ptr<IShowActivatable> CGuiHandler::topInt()
{
	if(listInt.empty())
		return std::shared_ptr<IShowActivatable>();
	else
		return listInt.front();
}

void CGuiHandler::totalRedraw()
{
#ifdef VCMI_ANDROID
	SDL_FillRect(screen2, NULL, SDL_MapRGB(screen2->format, 0, 0, 0));
#endif
	for(auto & elem : objsToBlit)
		elem->showAll(screen2);
	CSDL_Ext::blitAt(screen2,0,0,screen);
}

void CGuiHandler::updateTime()
{
	int ms = mainFPSmng->getElapsedMilliseconds();
	std::list<CIntObject*> hlp = timeinterested;
	for (auto & elem : hlp)
	{
		if(!vstd::contains(timeinterested,elem)) continue;
		(elem)->tick(ms);
	}
}

void CGuiHandler::convertMousePostition(int &x, int &y) 
{
	int w,h;
	SDL_GetWindowSize(mainWindow, &w, &h);
	float coef = 1.0 / screenCoef;
	if (nativeLandscape) {
		if (screenRotation == 0) {
			x = x * coef;
			y = y * coef;
		} 
		else if (screenRotation == 180) {
			x = (w - x) * coef;
			y = (h - y) * coef;
		}
	}
	else 
	{
		Sint32 tmp = x;
		if (screenRotation == 90) {
			x = y * coef;
			y = (w - tmp) * coef;
		} 
		else if (screenRotation == 270) {
			x = (h - y) * coef;
			y = tmp * coef;
		}
	}
	x = margins.x > x ? 0 : (x - margins.x);
}

void CGuiHandler::handleEvents()
{
	//player interface may want special event handling
	if(nullptr != LOCPLINT && LOCPLINT->capturedAllEvents())
		return;

	boost::unique_lock<boost::mutex> lock(eventsM);
	while(!SDLEventsQueue.empty())
	{
		continueEventHandling = true;
		SDL_Event currentEvent = SDLEventsQueue.front();

		if (currentEvent.type == SDL_MOUSEMOTION)
		{
#ifdef VCMI_AURORAOS
			convertMousePostition(currentEvent.motion.x, currentEvent.motion.y);
#endif
			cursorPosition = Point(currentEvent.motion.x, currentEvent.motion.y);
			mouseButtonsMask = currentEvent.motion.state;
		}
		SDLEventsQueue.pop();

		// In a sequence of mouse motion events, skip all but the last one.
		// This prevents freezes when every motion event takes longer to handle than interval at which
		// the events arrive (like dragging on the minimap in world view, with redraw at every event)
		// so that the events would start piling up faster than they can be processed.
		if ((currentEvent.type == SDL_MOUSEMOTION) && !SDLEventsQueue.empty() && (SDLEventsQueue.front().type == SDL_MOUSEMOTION))
			continue;

		handleCurrentEvent(currentEvent);
	}
}

void CGuiHandler::convertTouchToMouse(SDL_Event * current)
{
	int rLogicalWidth, rLogicalHeight;
#ifdef VCMI_AURORAOS
	rLogicalWidth = 1;
	rLogicalHeight = 1;
#else
	SDL_RenderGetLogicalSize(mainRenderer, &rLogicalWidth, &rLogicalHeight);
#endif

	int adjustedMouseY = (int)(current->tfinger.y * rLogicalHeight);
	int adjustedMouseX = (int)(current->tfinger.x * rLogicalWidth);

	current->button.x = adjustedMouseX;
	current->motion.x = adjustedMouseX;
	current->button.y = adjustedMouseY;
	current->motion.y = adjustedMouseY;
}

void CGuiHandler::fakeMoveCursor(float dx, float dy)
{
	int x, y, w, h;

	SDL_Event event;
	SDL_MouseMotionEvent sme = {SDL_MOUSEMOTION, 0, 0, 0, 0, 0, 0, 0, 0};

	sme.state = SDL_GetMouseState(&x, &y);
	SDL_GetWindowSize(mainWindow, &w, &h);

	sme.x = CCS->curh->position().x + (int)(GH.pointerSpeedMultiplier * w * dx);
	sme.y = CCS->curh->position().y + (int)(GH.pointerSpeedMultiplier * h * dy);

	vstd::abetween(sme.x, 0, w);
	vstd::abetween(sme.y, 0, h);

	event.motion = sme;
	SDL_PushEvent(&event);
}

void CGuiHandler::fakeMouseMove()
{
	fakeMoveCursor(0, 0);
}

void CGuiHandler::startTextInput(const Rect & whereInput)
{
#ifdef VCMI_APPLE
	dispatch_async(dispatch_get_main_queue(), ^{
#endif

	// TODO ios: looks like SDL bug actually, try fixing there
	auto renderer = SDL_GetRenderer(mainWindow);
	float scaleX, scaleY;
	SDL_Rect viewport;
	SDL_RenderGetScale(renderer, &scaleX, &scaleY);
	SDL_RenderGetViewport(renderer, &viewport);

#ifdef VCMI_IOS
	const auto nativeScale = iOS_utils::screenScale();
	scaleX /= nativeScale;
	scaleY /= nativeScale;
#endif

	SDL_Rect rectInScreenCoordinates;
	rectInScreenCoordinates.x = (viewport.x + whereInput.x) * scaleX;
	rectInScreenCoordinates.y = (viewport.y + whereInput.y) * scaleY;
	rectInScreenCoordinates.w = whereInput.w * scaleX;
	rectInScreenCoordinates.h = whereInput.h * scaleY;

	SDL_SetTextInputRect(&rectInScreenCoordinates);

	if (SDL_IsTextInputActive() == SDL_FALSE)
	{
		SDL_StartTextInput();
	}

#ifdef VCMI_APPLE
	});
#endif
}

void CGuiHandler::stopTextInput()
{
#ifdef VCMI_APPLE
	dispatch_async(dispatch_get_main_queue(), ^{
#endif

	if (SDL_IsTextInputActive() == SDL_TRUE)
	{
		SDL_StopTextInput();
	}

#ifdef VCMI_APPLE
	});
#endif
}

void CGuiHandler::fakeMouseButtonEventRelativeMode(bool down, bool right)
{
	SDL_Event event;
	SDL_MouseButtonEvent sme = {SDL_MOUSEBUTTONDOWN, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	if(!down)
	{
		sme.type = SDL_MOUSEBUTTONUP;
	}

	sme.button = right ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;

	sme.x = CCS->curh->position().x;
	sme.y = CCS->curh->position().y;

	float xScale, yScale;
	int w, h, rLogicalWidth, rLogicalHeight;

	SDL_GetWindowSize(mainWindow, &w, &h);
	SDL_RenderGetLogicalSize(mainRenderer, &rLogicalWidth, &rLogicalHeight);
	SDL_RenderGetScale(mainRenderer, &xScale, &yScale);

	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	moveCursorToPosition( Point(
		(int)(sme.x * xScale) + (w - rLogicalWidth * xScale) / 2,
		(int)(sme.y * yScale + (h - rLogicalHeight * yScale) / 2)));
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);

	event.button = sme;
	SDL_PushEvent(&event);
}

void CGuiHandler::handleCurrentEvent( SDL_Event & current )
{
	if(current.type == SDL_KEYDOWN || current.type == SDL_KEYUP)
	{
		SDL_KeyboardEvent key = current.key;

		if (key.repeat != 0)
			return; // ignore periodic event resends

		if(current.type == SDL_KEYDOWN && key.keysym.sym >= SDLK_F1 && key.keysym.sym <= SDLK_F15 && settings["session"]["spectate"].Bool())
		{
			//TODO: we need some central place for all interface-independent hotkeys
			Settings s = settings.write["session"];
			switch(key.keysym.sym)
			{
			case SDLK_F5:
				if(settings["session"]["spectate-locked-pim"].Bool())
					LOCPLINT->pim->unlock();
				else
					LOCPLINT->pim->lock();
				s["spectate-locked-pim"].Bool() = !settings["session"]["spectate-locked-pim"].Bool();
				break;

			case SDLK_F6:
				s["spectate-ignore-hero"].Bool() = !settings["session"]["spectate-ignore-hero"].Bool();
				break;

			case SDLK_F7:
				s["spectate-skip-battle"].Bool() = !settings["session"]["spectate-skip-battle"].Bool();
				break;

			case SDLK_F8:
				s["spectate-skip-battle-result"].Bool() = !settings["session"]["spectate-skip-battle-result"].Bool();
				break;

			case SDLK_F9:
				//not working yet since CClient::run remain locked after BattleInterface removal
//				if(LOCPLINT->battleInt)
//				{
//					GH.popInts(1);
//					vstd::clear_pointer(LOCPLINT->battleInt);
//				}
				break;

			default:
				break;
			}
			return;
		}

		auto shortcutsVector = shortcutsHandler().translateKeycode(key.keysym.sym);

		bool keysCaptured = false;
		for(auto i = keyinterested.begin(); i != keyinterested.end() && continueEventHandling; i++)
		{
			for (EShortcut shortcut : shortcutsVector)
			{
				if((*i)->captureThisKey(shortcut))
				{
					keysCaptured = true;
					break;
				}
			}
		}

		std::list<CIntObject*> miCopy = keyinterested;
		for(auto i = miCopy.begin(); i != miCopy.end() && continueEventHandling; i++)
		{
			for (EShortcut shortcut : shortcutsVector)
			{
				if(vstd::contains(keyinterested,*i) && (!keysCaptured || (*i)->captureThisKey(shortcut)))
				{
					if (key.state == SDL_PRESSED)
						(**i).keyPressed(shortcut);
					if (key.state == SDL_RELEASED)
						(**i).keyReleased(shortcut);
				}
			}
		}
	}
	else if(current.type == SDL_MOUSEMOTION)
	{
		handleMouseMotion(current);
	}
	else if(current.type == SDL_MOUSEBUTTONDOWN)
	{
#ifdef VCMI_AURORAOS
		convertMousePostition(current.button.x, current.button.y);
		cursorPosition.x = current.button.x;
		cursorPosition.y = current.button.y;
#endif
		switch(current.button.button)
		{
		case SDL_BUTTON_LEFT:
		{
			auto doubleClicked = false;
			if(lastClick == getCursorPosition() && (SDL_GetTicks() - lastClickTime) < 300)
			{
				std::list<CIntObject*> hlp = doubleClickInterested;
				for(auto i = hlp.begin(); i != hlp.end() && continueEventHandling; i++)
				{
					if(!vstd::contains(doubleClickInterested, *i)) continue;
					if((*i)->pos.isInside(current.motion.x, current.motion.y))
					{
						(*i)->onDoubleClick();
						doubleClicked = true;
					}
				}

			}

			lastClick = current.motion;
			lastClickTime = SDL_GetTicks();

			if(!doubleClicked)
				handleMouseButtonClick(lclickable, MouseButton::LEFT, true);
			break;
		}
		case SDL_BUTTON_RIGHT:
			handleMouseButtonClick(rclickable, MouseButton::RIGHT, true);
			break;
		case SDL_BUTTON_MIDDLE:
			handleMouseButtonClick(mclickable, MouseButton::MIDDLE, true);
			break;
		default:
			break;
		}
	}
	else if(current.type == SDL_MOUSEWHEEL)
	{
		std::list<CIntObject*> hlp = wheelInterested;
		for(auto i = hlp.begin(); i != hlp.end() && continueEventHandling; i++)
		{
			if(!vstd::contains(wheelInterested,*i)) continue;
			// SDL doesn't have the proper values for mouse positions on SDL_MOUSEWHEEL, refetch them
			int x = 0, y = 0;
			SDL_GetMouseState(&x, &y);
			(*i)->wheelScrolled(current.wheel.y < 0, (*i)->pos.isInside(x, y));
		}
	}
	else if(current.type == SDL_TEXTINPUT)
	{
		for(auto it : textInterested)
		{
			it->textInputed(current.text.text);
		}
	}
	else if(current.type == SDL_TEXTEDITING)
	{
		for(auto it : textInterested)
		{
			it->textEdited(current.edit.text);
		}
	}
	else if(current.type == SDL_MOUSEBUTTONUP)
	{
		if(!multifinger)
		{
			switch(current.button.button)
			{
			case SDL_BUTTON_LEFT:
				handleMouseButtonClick(lclickable, MouseButton::LEFT, false);
				break;
			case SDL_BUTTON_RIGHT:
				handleMouseButtonClick(rclickable, MouseButton::RIGHT, false);
				break;
			case SDL_BUTTON_MIDDLE:
				handleMouseButtonClick(mclickable, MouseButton::MIDDLE, false);
				break;
			}
		}
	}
	else if(current.type == SDL_FINGERMOTION)
	{
		if(isPointerRelativeMode)
		{
#ifdef VCMI_AURORAOS
			int w,h;
	 		SDL_GetWindowSize(mainWindow, &w, &h);
			fakeMoveCursor(current.tfinger.dx/w, current.tfinger.dy/h);
#else
			fakeMoveCursor(current.tfinger.dx, current.tfinger.dy);
#endif
		}
#ifdef VCMI_AURORAOS
		else
		{ // Emulate mouse event 
			int x, y, w, h;

			SDL_Event event;
			SDL_MouseMotionEvent sme = {SDL_MOUSEMOTION, 0, 0, 0, 0, 0, 0, 0, 0};

			sme.state = SDL_GetMouseState(&x, &y);
			SDL_GetWindowSize(mainWindow, &w, &h);

			sme.x = current.tfinger.x;
			sme.y = current.tfinger.y;

			event.motion = sme;
			SDL_PushEvent(&event);
		}
#endif
	}
	else if(current.type == SDL_FINGERDOWN)
	{
		auto fingerCount = SDL_GetNumTouchFingers(current.tfinger.touchId);

		multifinger = fingerCount > 1;
#ifdef VCMI_AURORAOS
		if (fingerCount == 1) {
			SDL_Event event;
			SDL_MouseButtonEvent nev;
			nev.type = SDL_MOUSEBUTTONDOWN;        /**< ::SDL_MOUSEBUTTONDOWN or ::SDL_MOUSEBUTTONUP */
			// nev.timestamp = current.tfinger.timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
			// nev.windowID = current.tfinger.windowID;    /**< The window with mouse focus, if any */
			// nev.which = current.tfinger.touchId;       /**< The mouse instance id, or SDL_TOUCH_MOUSEID */
			nev.button = SDL_BUTTON_LEFT;       /**< The mouse button index */
			nev.state = SDL_PRESSED;        /**< ::SDL_PRESSED or ::SDL_RELEASED */
			nev.clicks = 1;       /**< 1 for single-click, 2 for double-click, etc. */
			// nev.padding1 = 0 ;
			nev.x = current.tfinger.x;           /**< X coordinate, relative to window */
			nev.y = current.tfinger.y;           /**< Y coordinate, relative to window */
			event.button = nev;
			SDL_PushEvent(&event);
		}
#endif
		if(isPointerRelativeMode)
		{
#ifdef VCMI_AURORAOS
			int w,h;
			SDL_GetWindowSize(mainWindow, &w, &h);
			current.tfinger.x = current.tfinger.x/w;
			current.tfinger.x = current.tfinger.x/h;
#endif

			if(current.tfinger.x > 0.5)
			{
				bool isRightClick = current.tfinger.y < 0.5;

				fakeMouseButtonEventRelativeMode(true, isRightClick);
			}
		}
#ifndef VCMI_IOS
		else if(fingerCount == 2)
		{
			convertTouchToMouse(&current);
			handleMouseMotion(current);
			handleMouseButtonClick(rclickable, MouseButton::RIGHT, true);
		}
#endif //VCMI_IOS
	}
	else if(current.type == SDL_FINGERUP)
	{
#ifndef VCMI_IOS
		auto fingerCount = SDL_GetNumTouchFingers(current.tfinger.touchId);
#endif //VCMI_IOS

#ifdef VCMI_AURORAOS
		if (fingerCount == 0) {
			SDL_Event event;
			SDL_MouseButtonEvent nev;
			nev.type = SDL_MOUSEBUTTONUP;        /**< ::SDL_MOUSEBUTTONDOWN or ::SDL_MOUSEBUTTONUP */
			// nev.timestamp = current.tfinger.timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
			// nev.windowID = current.tfinger.windowID;    /**< The window with mouse focus, if any */
			// nev.which = current.tfinger.touchId;       /**< The mouse instance id, or SDL_TOUCH_MOUSEID */
			nev.button = SDL_BUTTON_LEFT;       /**< The mouse button index */
			nev.state = SDL_RELEASED;        /**< ::SDL_PRESSED or ::SDL_RELEASED */
			nev.clicks = 1;       /**< 1 for single-click, 2 for double-click, etc. */
// 			nev.padding1 = 0 ;
			nev.x = current.tfinger.x;           /**< X coordinate, relative to window */
			nev.y = current.tfinger.y;           /**< Y coordinate, relative to window */
			event.button = nev;
			SDL_PushEvent(&event);
		}
#endif
		if(isPointerRelativeMode)
		{
#ifdef VCMI_AURORAOS
			int w,h;
			SDL_GetWindowSize(mainWindow, &w, &h);
			current.tfinger.x = current.tfinger.x/w;
			current.tfinger.x = current.tfinger.x/h;
#endif
			if(current.tfinger.x > 0.5)
			{
				bool isRightClick = current.tfinger.y < 0.5;

				fakeMouseButtonEventRelativeMode(false, isRightClick);
			}
		}
#ifndef VCMI_IOS
		else if(multifinger)
		{
			convertTouchToMouse(&current);
			handleMouseMotion(current);
			handleMouseButtonClick(rclickable, MouseButton::RIGHT, false);
			multifinger = fingerCount != 0;
		}
#endif //VCMI_IOS
	}
#ifdef VCMI_AURORAOS
	else if(current.type == SDL_MULTIGESTURE) 
	{
		if (current.mgesture.numFingers == 2) {
			screenScale  = current.mgesture.dDist;
			pinPoint.x = current.mgesture.x;
			pinPoint.y = current.mgesture.y;
		}
	}
#endif
} //event end

void CGuiHandler::handleMouseButtonClick(CIntObjectList & interestedObjs, MouseButton btn, bool isPressed)
{
	auto hlp = interestedObjs;
	for(auto i = hlp.begin(); i != hlp.end() && continueEventHandling; i++)
	{
		if(!vstd::contains(interestedObjs, *i)) continue;

		auto prev = (*i)->mouseState(btn);
		if(!isPressed)
			(*i)->updateMouseState(btn, isPressed);
		if((*i)->pos.isInside(getCursorPosition()))
		{
			if(isPressed)
				(*i)->updateMouseState(btn, isPressed);
			(*i)->click(btn, isPressed, prev);
		}
		else if(!isPressed)
			(*i)->click(btn, boost::logic::indeterminate, prev);
	}
}

void CGuiHandler::handleMouseMotion(const SDL_Event & current)
{
	//sending active, hovered hoverable objects hover() call
	std::vector<CIntObject*> hlp;

	auto hoverableCopy = hoverable;
	for(auto & elem : hoverableCopy)
	{
		if(elem->pos.isInside(getCursorPosition()))
		{
			if (!(elem)->hovered)
				hlp.push_back((elem));
		}
		else if ((elem)->hovered)
		{
			(elem)->hover(false);
			(elem)->hovered = false;
		}
	}

	for(auto & elem : hlp)
	{
		elem->hover(true);
		elem->hovered = true;
	}

	// do not send motion events for events outside our window
	//if (current.motion.windowID == 0)
		handleMoveInterested(current.motion);
}

void CGuiHandler::simpleRedraw()
{
	//update only top interface and draw background
	if(objsToBlit.size() > 1)
		CSDL_Ext::blitAt(screen2,0,0,screen); //blit background
	if(!objsToBlit.empty())
		objsToBlit.back()->show(screen); //blit active interface/window
}

void CGuiHandler::handleMoveInterested(const SDL_MouseMotionEvent & motion)
{
	//sending active, MotionInterested objects mouseMoved() call
	std::list<CIntObject*> miCopy = motioninterested;
	for(auto & elem : miCopy)
	{
		if(elem->strongInterest || Rect::createAround(elem->pos, 1).isInside( motion.x, motion.y)) //checking bounds including border fixes bug #2476
		{
			(elem)->mouseMoved(Point(motion.x, motion.y));
		}
	}
}

void CGuiHandler::setScreenOrientation(int orientation, void *sdlWindow) {
	// screenOrientation = orientation;
	struct SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (SDL_GetWindowWMInfo((SDL_Window*)sdlWindow, &wmInfo)) 
	{
		int w,h;
		SDL_GetWindowSize(mainWindow, &w, &h);

		if (nativeLandscape) 
		{
			// NativeLandscape
			margins.x = ((float)w / screenCoef - (float)screen->w) * 0.5;
			margins.y = 0;

			if (orientation == SDL_ORIENTATION_PORTRAIT) 
			{
				screenRotation = 0;
				wl_surface_set_buffer_transform(wmInfo.info.wl.surface, WL_OUTPUT_TRANSFORM_NORMAL);
				logGlobal->info("[AuroraOS Debug] screenOrientation SDL_ORIENTATION_PORTRAIT");
			}
			else if (orientation == SDL_ORIENTATION_PORTRAIT_FLIPPED)
			{
				screenRotation = 180;
				wl_surface_set_buffer_transform(wmInfo.info.wl.surface, WL_OUTPUT_TRANSFORM_180);
				logGlobal->info("[AuroraOS Debug] screenOrientation SDL_ORIENTATION_PORTRAIT_FLIPPED");
			}
		} 
		else 
		{
			margins.x = ((float)h / screenCoef - (float)screen->w) * 0.5;
			margins.y = 0;

			if (orientation == SDL_ORIENTATION_LANDSCAPE) 
			{
				screenRotation = 90;
				wl_surface_set_buffer_transform(wmInfo.info.wl.surface, WL_OUTPUT_TRANSFORM_270);
				logGlobal->info("[AuroraOS Debug] screenOrientation SDL_ORIENTATION_LANDSCAPE");
			}
			else if (orientation == SDL_ORIENTATION_LANDSCAPE_FLIPPED)
			{
				screenRotation = 270;
				wl_surface_set_buffer_transform(wmInfo.info.wl.surface, WL_OUTPUT_TRANSFORM_90);
				logGlobal->info("[AuroraOS Debug] screenOrientation SDL_ORIENTATION_LANDSCAPE_FLIPPED");
			}
		}
		logGlobal->info("[AuroraOS Debug] screenMargins %d x %d", margins.x, margins.y);
	}
}

void CGuiHandler::renderFrame()
{

	// Updating GUI requires locking pim mutex (that protects screen and GUI state).
	// During game:
	// When ending the game, the pim mutex might be hold by other thread,
	// that will notify us about the ending game by setting terminate_cond flag.
	//in PreGame terminate_cond stay false

	bool acquiredTheLockOnPim = false; //for tracking whether pim mutex locking succeeded
	while(!terminate_cond->get() && !(acquiredTheLockOnPim = CPlayerInterface::pim->try_lock())) //try acquiring long until it succeeds or we are told to terminate
		boost::this_thread::sleep(boost::posix_time::milliseconds(1));

	if(acquiredTheLockOnPim)
	{
		// If we are here, pim mutex has been successfully locked - let's store it in a safe RAII lock.
		boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim, boost::adopt_lock);

		if(nullptr != curInt)
			curInt->update();

		if(settings["video"]["showfps"].Bool())
			drawFPSCounter();

		SDL_UpdateTexture(screenTexture, nullptr, screen->pixels, screen->pitch);

		SDL_RenderClear(mainRenderer);
#ifdef VCMI_AURORAOS
		SDL_DisplayMode m;
		SDL_GetDesktopDisplayMode(0, &m);
		SDL_FRect dstr;
		// TODO calculate dst rect when orientation changed (and use it when convert mouse position)
		dstr = {
			(m.w - screen->w * screenCoef) * 0.5,
			(m.h - screen->h * screenCoef) * 0.5,
			screenCoef * screen->w,
			screenCoef * screen->h
		};

		SDL_RenderCopyExF(mainRenderer, screenTexture, nullptr, &dstr, screenRotation, nullptr, SDL_FLIP_NONE);
		CCS->curh->setCursorScale(screenCoef);
		CCS->curh->setCursorRotation(screenRotation);
#else
		SDL_RenderCopy(mainRenderer, screenTexture, nullptr, nullptr);
#endif
		CCS->curh->render();

		SDL_RenderPresent(mainRenderer);

		disposed.clear();
	}

	mainFPSmng->framerateDelay(); // holds a constant FPS
}


CGuiHandler::CGuiHandler()
	: lastClick(-500, -500)
	, lastClickTime(0)
	, defActionsDef(0)
	, captureChildren(false)
	, multifinger(false)
	, mouseButtonsMask(0)
	, continueEventHandling(true)
	, curInt(nullptr)
	, mainFPSmng(nullptr)
	, statusbar(nullptr)
{
	terminate_cond = new CondSh<bool>(false);
}

CGuiHandler::~CGuiHandler()
{
	delete mainFPSmng;
	delete terminate_cond;
}

ShortcutHandler & CGuiHandler::shortcutsHandler()
{
	return *shortcutsHandlerInstance;
}

void CGuiHandler::moveCursorToPosition(const Point & position)
{
	SDL_WarpMouseInWindow(mainWindow, position.x, position.y);
}

bool CGuiHandler::isKeyboardCtrlDown() const
{
#ifdef VCMI_MAC
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LGUI] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RGUI];
#else
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RCTRL];
#endif
}

bool CGuiHandler::isKeyboardAltDown() const
{
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LALT] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RALT];
}

bool CGuiHandler::isKeyboardShiftDown() const
{
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RSHIFT];
}

void CGuiHandler::breakEventHandling()
{
	continueEventHandling = false;
}

const Point & CGuiHandler::getCursorPosition() const
{
	return cursorPosition;
}

Point CGuiHandler::screenDimensions() const
{
	return Point(screen->w, screen->h);
}

bool CGuiHandler::isMouseButtonPressed() const
{
	return mouseButtonsMask > 0;
}

bool CGuiHandler::isMouseButtonPressed(MouseButton button) const
{
	static_assert(static_cast<uint32_t>(MouseButton::LEFT)   == SDL_BUTTON_LEFT,   "mismatch between VCMI and SDL enum!");
	static_assert(static_cast<uint32_t>(MouseButton::MIDDLE) == SDL_BUTTON_MIDDLE, "mismatch between VCMI and SDL enum!");
	static_assert(static_cast<uint32_t>(MouseButton::RIGHT)  == SDL_BUTTON_RIGHT,  "mismatch between VCMI and SDL enum!");
	static_assert(static_cast<uint32_t>(MouseButton::EXTRA1) == SDL_BUTTON_X1,     "mismatch between VCMI and SDL enum!");
	static_assert(static_cast<uint32_t>(MouseButton::EXTRA2) == SDL_BUTTON_X2,     "mismatch between VCMI and SDL enum!");

	uint32_t index = static_cast<uint32_t>(button);
	return mouseButtonsMask & SDL_BUTTON(index);
}

void CGuiHandler::drawFPSCounter()
{
	static SDL_Rect overlay = { 0, 0, 64, 32};
	uint32_t black = SDL_MapRGB(screen->format, 10, 10, 10);
	SDL_FillRect(screen, &overlay, black);
	std::string fps = std::to_string(mainFPSmng->getFramerate());
	graphics->fonts[FONT_BIG]->renderTextLeft(screen, fps, Colors::YELLOW, Point(10, 10));
}

bool CGuiHandler::amIGuiThread()
{
	return inGuiThread.get() && *inGuiThread;
}

void CGuiHandler::pushUserEvent(EUserEvent usercode)
{
	pushUserEvent(usercode, nullptr);
}

void CGuiHandler::pushUserEvent(EUserEvent usercode, void * userdata)
{
	SDL_Event event;
	event.type = SDL_USEREVENT;
	event.user.code = static_cast<int32_t>(usercode);
	event.user.data1 = userdata;
	SDL_PushEvent(&event);
}

CFramerateManager::CFramerateManager()
	: rate(0)
	, rateticks(0)
	, fps(0)
	, accumulatedFrames(0)
	, accumulatedTime(0)
	, lastticks(0)
	, timeElapsed(0)
{}

void CFramerateManager::init(int newRate)
{
	rate = newRate;
	rateticks = 1000.0 / rate;
	this->lastticks = SDL_GetTicks();
}

void CFramerateManager::framerateDelay()
{
	ui32 currentTicks = SDL_GetTicks();

	timeElapsed = currentTicks - lastticks;
	accumulatedFrames++;

	// FPS is higher than it should be, then wait some time
	if(timeElapsed < rateticks)
	{
		int timeToSleep = (uint32_t)ceil(this->rateticks) - timeElapsed;
		boost::this_thread::sleep(boost::posix_time::milliseconds(timeToSleep));
	}

	currentTicks = SDL_GetTicks();
	// recalculate timeElapsed for external calls via getElapsed()
	// limit it to 100 ms to avoid breaking animation in case of huge lag (e.g. triggered breakpoint)
	timeElapsed = std::min<ui32>(currentTicks - lastticks, 100);

	lastticks = SDL_GetTicks();

	accumulatedTime += timeElapsed;

	if(accumulatedFrames >= 100)
	{
		//about 2 second should be passed
		fps = static_cast<int>(ceil(1000.0 / (accumulatedTime / accumulatedFrames)));
		accumulatedTime = 0;
		accumulatedFrames = 0;
	}
}
