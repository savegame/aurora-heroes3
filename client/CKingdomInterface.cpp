#include "StdInc.h"
#include "CKingdomInterface.h"

#include "../CCallback.h"
#include "../lib/CCreatureHandler.h" //creatures name for objects list
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CModHandler.h" //for buildings per turn
#include "../lib/CObjectHandler.h" //Hero/Town objects
#include "../lib/CHeroHandler.h" // only for calculating required xp? worth it?
#include "../lib/CTownHandler.h"
#include "CAnimation.h" //CAnimImage
#include "CAdvmapInterface.h" //CResDataBar
#include "CCastleInterface.h" //various town-specific classes
#include "../lib/CConfigHandler.h"
#include "CGameInfo.h"
#include "CPlayerInterface.h" //LOCPLINT
#include "UIFramework/CGuiHandler.h"
#include "UIFramework/CIntObjectClasses.h"

/*
 * CKingdomInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern SDL_Surface *screenBuf;

InfoBox::InfoBox(Point position, InfoPos Pos, InfoSize Size, IInfoBoxData *Data):
	size(Size),
	infoPos(Pos),
	data(Data),
	value(NULL),
	name(NULL)
{
	assert(data);
	addUsedEvents(LCLICK | RCLICK);
	EFonts font = (size < SIZE_MEDIUM)? FONT_SMALL: FONT_MEDIUM;

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos+=position;

	image = new CAnimImage(data->getImageName(size), data->getImageIndex());
	pos = image->pos;

	if (infoPos == POS_CORNER)
		value = new CLabel(pos.w, pos.h, font, BOTTOMRIGHT, Colors::WHITE, data->getValueText());

	if (infoPos == POS_INSIDE)
		value = new CLabel(pos.w/2, pos.h-6, font, CENTER, Colors::WHITE, data->getValueText());

	if (infoPos == POS_UP_DOWN || infoPos == POS_DOWN)
		value = new CLabel(pos.w/2, pos.h+8, font, CENTER, Colors::WHITE, data->getValueText());

	if (infoPos == POS_UP_DOWN)
		name = new CLabel(pos.w/2, -12, font, CENTER, Colors::WHITE, data->getNameText());

	if (infoPos == POS_RIGHT)
	{
		name = new CLabel(pos.w+6, 6, font, TOPLEFT, Colors::WHITE, data->getNameText());
		value = new CLabel(pos.w+6, pos.h-16, font, TOPLEFT, Colors::WHITE, data->getValueText());
	}
	pos = image->pos;
	if (name)
		pos = pos | name->pos;
	if (value)
		pos = pos | value->pos;
	
	hover = new CHoverableArea;
	hover->hoverText = data->getHoverText();
	hover->pos = pos;
}

InfoBox::~InfoBox()
{
	delete data;
}

void InfoBox::clickRight(tribool down, bool previousState)
{
	if (down)
	{
		CComponent *comp = nullptr;
		std::string text;
		data->prepareMessage(text, &comp);
		if (comp)
			CRClickPopup::createAndPush(text, CInfoWindow::TCompsInfo(1, comp));
		else if (!text.empty())
			adventureInt->handleRightClick(text, down);
	}
}

void InfoBox::clickLeft(tribool down, bool previousState)
{
	if((!down) && previousState)
	{
		CComponent *comp = nullptr;
		std::string text;
		data->prepareMessage(text, &comp);

		std::vector<CComponent*> compVector;
		if (comp)
		{
			compVector.push_back(comp);
			LOCPLINT->showInfoDialog(text, compVector);
		}
	}
}

//TODO?
/*
void InfoBox::update()
{

}
*/

IInfoBoxData::IInfoBoxData(InfoType Type):
	type(Type)
{
}

InfoBoxAbstractHeroData::InfoBoxAbstractHeroData(InfoType Type):
	IInfoBoxData(Type)
{
}

std::string InfoBoxAbstractHeroData::getValueText()
{
	switch (type)
	{
	case HERO_MANA:
	case HERO_EXPERIENCE:
	case HERO_PRIMARY_SKILL:
		return boost::lexical_cast<std::string>(getValue());
	case HERO_SPECIAL:
		{
			std::string text = CGI->generaltexth->jktexts[5];
			size_t begin = text.find('{');
			size_t end   = text.find('}', begin);
			return text.substr(begin, end-begin);
		}
	case HERO_SECONDARY_SKILL:
		{
			si64 value = getValue();
			if (value)
				return CGI->generaltexth->levels[value];
		}
	default:
		assert(0);
	}
	return "";
}

std::string InfoBoxAbstractHeroData::getNameText()
{
	switch (type)
	{
	case HERO_PRIMARY_SKILL:
		return CGI->generaltexth->primarySkillNames[getSubID()];
	case HERO_MANA:
		return CGI->generaltexth->allTexts[387];
	case HERO_EXPERIENCE:
		{
			std::string text = CGI->generaltexth->jktexts[6];
			size_t begin = text.find('{');
			size_t end   = text.find('}', begin);
			return text.substr(begin, end-begin);
		}
	case HERO_SPECIAL:
		return CGI->heroh->heroes[getSubID()]->specName;
	case HERO_SECONDARY_SKILL:
		if (getValue())
			return CGI->generaltexth->skillName[getSubID()];
		else
			return "";
	default:
		assert(0);
	}
	return "";
}

std::string InfoBoxAbstractHeroData::getImageName(InfoBox::InfoSize size)
{
	//TODO: sizes
	switch(size)
	{
	case InfoBox::SIZE_SMALL:
		{
			switch(type)
			{
			case HERO_PRIMARY_SKILL:
			case HERO_MANA:
			case HERO_EXPERIENCE:
				return "PSKIL32";
			case HERO_SPECIAL:
				return "UN32";
			case HERO_SECONDARY_SKILL:
				return "SECSK32";
			default:
				assert(0);
			}
		}
	case InfoBox::SIZE_BIG:
		{
			switch(type)
			{
			case HERO_PRIMARY_SKILL:
			case HERO_MANA:
			case HERO_EXPERIENCE:
				return "PSKIL42";
			case HERO_SPECIAL:
				return "UN44";
			case HERO_SECONDARY_SKILL:
				return "SECSKILL";
			default:
				assert(0);
			}
		}
	default:
		assert(0);
	}
	return "";
}

std::string InfoBoxAbstractHeroData::getHoverText()
{
	//TODO: any texts here?
	return "";
}

size_t InfoBoxAbstractHeroData::getImageIndex()
{
	switch (type)
	{
	case HERO_SPECIAL:
		return VLC->heroh->heroes[getSubID()]->imageIndex;
	case HERO_PRIMARY_SKILL:
		return getSubID();
	case HERO_MANA:
		return 5;
	case HERO_EXPERIENCE:
		return 4;
	case HERO_SECONDARY_SKILL:
		{
			si64 value = getValue();
			if (value)
				return getSubID()*3 + value + 2;
			else
				return 0;//FIXME: Should be transparent instead of empty
		}
	default:
		assert(0);
		return 0;
	}
}

bool InfoBoxAbstractHeroData::prepareMessage(std::string &text, CComponent **comp)
{
	switch (type)
	{
	case HERO_SPECIAL:
		text = CGI->heroh->heroes[getSubID()]->specDescr;
		*comp = NULL;
		return true;
	case HERO_PRIMARY_SKILL:
		text = CGI->generaltexth->arraytxt[2+getSubID()];
		*comp =new CComponent(CComponent::primskill, getSubID(), getValue());
		return true;
	case HERO_MANA:
		text = CGI->generaltexth->allTexts[149];
		*comp = NULL;
		return true;
	case HERO_EXPERIENCE:
		text = CGI->generaltexth->allTexts[241];
		*comp = NULL;
		return true;
	case HERO_SECONDARY_SKILL:
		{
			si64 value = getValue();
			int  subID = getSubID();
			if (!value)
				return false;

			text = CGI->generaltexth->skillInfoTexts[subID][value-1];
			*comp = new CComponent(CComponent::secskill, subID, value);
			return true;
		}
	default:
		assert(0);
		return false;
	}
}

InfoBoxHeroData::InfoBoxHeroData(InfoType Type, const CGHeroInstance * Hero, int Index):
	InfoBoxAbstractHeroData(Type),
	hero(Hero),
	index(Index)
{
}

int InfoBoxHeroData::getSubID()
{
	switch(type)
	{
		case HERO_PRIMARY_SKILL:
			return index;
		case HERO_SECONDARY_SKILL:
			if (hero->secSkills.size() > index)
				return hero->secSkills[index].first;
		case HERO_MANA:
		case HERO_EXPERIENCE:
		case HERO_SPECIAL:
			return 0;
		default:
			assert(0);
			return 0;
	}
}

si64 InfoBoxHeroData::getValue()
{
	switch(type)
	{
	case HERO_PRIMARY_SKILL:
		return hero->getPrimSkillLevel(index);
	case HERO_MANA:
		return hero->mana;
	case HERO_EXPERIENCE:
		return hero->exp;
	case HERO_SECONDARY_SKILL:
		if (hero->secSkills.size() > index)
			return hero->secSkills[index].second;
	case HERO_SPECIAL:
			return 0;
	default:
		assert(0);
		return 0;
	}
}

std::string InfoBoxHeroData::getHoverText()
{
	switch (type)
	{
	case HERO_PRIMARY_SKILL:
		return boost::str(boost::format(CGI->generaltexth->heroscrn[1]) % CGI->generaltexth->primarySkillNames[index]);
	case HERO_MANA:
		return CGI->generaltexth->heroscrn[22];
	case HERO_EXPERIENCE:
		return CGI->generaltexth->heroscrn[9];
	case HERO_SPECIAL:
		return CGI->generaltexth->heroscrn[27];
	case HERO_SECONDARY_SKILL:
		{
		if (hero->secSkills.size() > index)
		{
			std::string level = CGI->generaltexth->levels[hero->secSkills[index].second-1];
			std::string skill = CGI->generaltexth->skillName[hero->secSkills[index].first];
			return boost::str(boost::format(CGI->generaltexth->heroscrn[21]) % level % skill);
		}
		else
			return "";
		}
	default:
		return InfoBoxAbstractHeroData::getHoverText();
	}
}

std::string InfoBoxHeroData::getValueText()
{
	switch (type)
	{
	case HERO_MANA:
		if (hero)
			return boost::lexical_cast<std::string>(hero->mana) + '/' +
			       boost::lexical_cast<std::string>(hero->manaLimit());
	case HERO_EXPERIENCE:
		return boost::lexical_cast<std::string>(hero->exp);
	default:
		return InfoBoxAbstractHeroData::getValueText();
	}
}

bool InfoBoxHeroData::prepareMessage(std::string &text, CComponent**comp)
{
	switch(type)
	{
	case HERO_MANA:
		text = CGI->generaltexth->allTexts[205];
		boost::replace_first(text, "%s", boost::lexical_cast<std::string>(hero->name));
		boost::replace_first(text, "%d", boost::lexical_cast<std::string>(hero->mana));
		boost::replace_first(text, "%d", boost::lexical_cast<std::string>(hero->manaLimit()));
		*comp = NULL;
		return true;

	case HERO_EXPERIENCE:
		text = CGI->generaltexth->allTexts[2];
		boost::replace_first(text, "%d", boost::lexical_cast<std::string>(hero->level));
		boost::replace_first(text, "%d", boost::lexical_cast<std::string>(CGI->heroh->reqExp(hero->level+1)));
		boost::replace_first(text, "%d", boost::lexical_cast<std::string>(hero->exp));
		*comp = NULL;
		return true;

	default:
		return InfoBoxAbstractHeroData::prepareMessage(text, comp);
	}
}

InfoBoxCustomHeroData::InfoBoxCustomHeroData(InfoType Type, int SubID, si64 Value):
	InfoBoxAbstractHeroData(Type),
	subID(SubID),
	value(Value)
{
}

int InfoBoxCustomHeroData::getSubID()
{
	return subID;
}

si64 InfoBoxCustomHeroData::getValue()
{
	return value;
}

InfoBoxCustom::InfoBoxCustom(std::string ValueText, std::string NameText, std::string ImageName, size_t ImageIndex, std::string HoverText):
	IInfoBoxData(CUSTOM),
	valueText(ValueText),
	nameText(NameText),
	imageName(ImageName),
	hoverText(HoverText),
	imageIndex(ImageIndex)
{
}

std::string InfoBoxCustom::getHoverText()
{
	return hoverText;
}

size_t InfoBoxCustom::getImageIndex()
{
	return imageIndex;
}

std::string InfoBoxCustom::getImageName(InfoBox::InfoSize size)
{
	return imageName;
}

std::string InfoBoxCustom::getNameText()
{
	return nameText;
}

std::string InfoBoxCustom::getValueText()
{
	return valueText;
}

bool InfoBoxCustom::prepareMessage(std::string &text, CComponent **comp)
{
	return false;
}

CKingdomInterface::CKingdomInterface():
    CWindowObject(PLAYER_COLORED | BORDERED, conf.go()->ac.overviewBg)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	ui32 footerPos = conf.go()->ac.overviewSize * 116;

	tabArea = new CTabbedInt(boost::bind(&CKingdomInterface::createMainTab, this, _1), CTabbedInt::DestroyFunc(), Point(4,4));

	std::vector<const CGObjectInstance * > ownedObjects = LOCPLINT->cb->getMyObjects();
	generateObjectsList(ownedObjects);
	generateMinesList(ownedObjects);
	generateButtons();

	statusbar = new CGStatusBar(new CPicture("KSTATBAR", 10,pos.h - 45));
	resdatabar= new CResDataBar("KRESBAR", 3, 111+footerPos, 32, 2, 76, 76);
}

void CKingdomInterface::generateObjectsList(const std::vector<const CGObjectInstance * > &ownedObjects)
{
	ui32 footerPos = conf.go()->ac.overviewSize * 116;
	size_t dwellSize = (footerPos - 64)/57;

	//Map used to determine image number for several objects
	std::map<std::pair<int,int>,int> idToImage;
	idToImage[std::make_pair( 20, 1)] = 81;//Golem factory
	idToImage[std::make_pair( 42, 0)] = 82;//Lighthouse
	idToImage[std::make_pair( 33, 0)] = 83;//Garrison
	idToImage[std::make_pair(219, 0)] = 83;//Garrison
	idToImage[std::make_pair( 33, 1)] = 84;//Anti-magic Garrison
	idToImage[std::make_pair(219, 1)] = 84;//Anti-magic Garrison
	idToImage[std::make_pair( 53, 7)] = 85;//Abandoned mine
	idToImage[std::make_pair( 20, 0)] = 86;//Conflux
	idToImage[std::make_pair( 87, 0)] = 87;//Harbor

	std::map<int, OwnedObjectInfo> visibleObjects;
	BOOST_FOREACH(const CGObjectInstance * object, ownedObjects)
	{
		//Dwellings
		if ( object->ID == Obj::CREATURE_GENERATOR1 )
		{
			OwnedObjectInfo &info = visibleObjects[object->subID];
			if (info.count++ == 0)
			{
				info.hoverText = CGI->creh->creatures[CGI->objh->cregens.find(object->subID)->second]->namePl;
				info.imageID = object->subID;
			}
		}
		//Special objects from idToImage map that should be displayed in objects list
		std::map<std::pair<int,int>,int>::iterator iter = idToImage.find(std::make_pair(object->ID, object->subID));
		if (iter != idToImage.end())
		{
			OwnedObjectInfo &info = visibleObjects[iter->second];
			if (info.count++ == 0)
			{
				info.hoverText = object->hoverName;
				info.imageID = iter->second;
			}
		}
	}
	objects.reserve(visibleObjects.size());

	std::pair<int, OwnedObjectInfo> element;
	BOOST_FOREACH(element, visibleObjects)
	{
		objects.push_back(element.second);
	}
	dwellingsList = new CListBox(boost::bind(&CKingdomInterface::createOwnedObject, this, _1), CListBox::DestroyFunc(),
	                             Point(740,44), Point(0,57), dwellSize, visibleObjects.size());
}

CIntObject* CKingdomInterface::createOwnedObject(size_t index)
{
	if (index < objects.size())
	{
		OwnedObjectInfo &obj = objects[index];
		std::string value = boost::lexical_cast<std::string>(obj.count);
		return new InfoBox(Point(), InfoBox::POS_CORNER, InfoBox::SIZE_SMALL,
			   new InfoBoxCustom(value,"", "FLAGPORT", obj.imageID, obj.hoverText));
	}
	return NULL;
}

CIntObject * CKingdomInterface::createMainTab(size_t index)
{
	size_t size = conf.go()->ac.overviewSize;
	switch (index)
	{
	case 0: return new CKingdHeroList(size);
	case 1: return new CKingdTownList(size);
	default:return NULL;
	}
}

void CKingdomInterface::generateMinesList(const std::vector<const CGObjectInstance * > &ownedObjects)
{
	ui32 footerPos = conf.go()->ac.overviewSize * 116;
	std::vector<int> minesCount(GameConstants::RESOURCE_QUANTITY, 0);
	int totalIncome=0;

	BOOST_FOREACH(const CGObjectInstance * object, ownedObjects)
	{
		//Mines
		if ( object->ID == Obj::MINE )
		{
			const CGMine *mine = dynamic_cast<const CGMine*>(object);
			assert(mine);
			minesCount[mine->producedResource]++;

			if (mine->producedResource == Res::GOLD)
				totalIncome += mine->producedQuantity;
		}
	}

	//Heroes can produce gold as well - skill, speciality or arts
	std::vector<const CGHeroInstance*> heroes = LOCPLINT->cb->getHeroesInfo(true);
	for(size_t i=0; i<heroes.size(); i++)
	{
		totalIncome += heroes[i]->valOfBonuses(Selector::typeSubtype(Bonus::SECONDARY_SKILL_PREMY, CGHeroInstance::ESTATES));
		totalIncome += heroes[i]->valOfBonuses(Selector::typeSubtype(Bonus::GENERATE_RESOURCE, Res::GOLD));
	}

	//Add town income of all towns
	std::vector<const CGTownInstance*> towns = LOCPLINT->cb->getTownsInfo(true);
	for(size_t i=0; i<towns.size(); i++)
	{
		totalIncome += towns[i]->dailyIncome();
	}
	for (int i=0; i<7; i++)
	{
		std::string value = boost::lexical_cast<std::string>(minesCount[i]);
		minesBox[i] = new InfoBox(Point(20+i*80, 31+footerPos), InfoBox::POS_INSIDE, InfoBox::SIZE_SMALL,
		              new InfoBoxCustom(value, "", "OVMINES", i, CGI->generaltexth->mines[i].first));

		minesBox[i]->removeUsedEvents(LCLICK|RCLICK); //fixes #890 - mines boxes ignore clicks
	}
	incomeArea = new CHoverableArea;
	incomeArea->pos = Rect(pos.x+580, pos.y+31+footerPos, 136, 68);
	incomeArea->hoverText = CGI->generaltexth->allTexts[255];
	incomeAmount = new CLabel(628, footerPos + 70, FONT_SMALL, TOPLEFT, Colors::WHITE, boost::lexical_cast<std::string>(totalIncome));
}

void CKingdomInterface::generateButtons()
{
	ui32 footerPos = conf.go()->ac.overviewSize * 116;

	//Main control buttons
	btnHeroes = new CAdventureMapButton (CGI->generaltexth->overview[11], CGI->generaltexth->overview[6],
	                                    boost::bind(&CKingdomInterface::activateTab, this, 0),748,28+footerPos,"OVBUTN1.DEF", SDLK_h);
	btnHeroes->block(true);

	btnTowns = new CAdventureMapButton (CGI->generaltexth->overview[12], CGI->generaltexth->overview[7],
	                                   boost::bind(&CKingdomInterface::activateTab, this, 1),748,64+footerPos,"OVBUTN6.DEF", SDLK_t);

	btnExit = new CAdventureMapButton (CGI->generaltexth->allTexts[600],"",
	                                  boost::bind(&CKingdomInterface::close, this),748,99+footerPos,"OVBUTN1.DEF", SDLK_RETURN);
	btnExit->assignedKeys.insert(SDLK_ESCAPE);
	btnExit->setOffset(3);

	//Object list control buttons
	dwellTop = new CAdventureMapButton ("", "", boost::bind(&CListBox::moveToPos, dwellingsList, 0),
	                                   733, 4, "OVBUTN4.DEF");

	dwellBottom = new CAdventureMapButton ("", "", boost::bind(&CListBox::moveToPos, dwellingsList, -1),
	                                      733, footerPos+2, "OVBUTN4.DEF");
	dwellBottom->setOffset(2);

	dwellUp = new CAdventureMapButton ("", "", boost::bind(&CListBox::moveToPrev, dwellingsList),
	                                  733, 24, "OVBUTN4.DEF");
	dwellUp->setOffset(4);

	dwellDown = new CAdventureMapButton ("", "", boost::bind(&CListBox::moveToNext, dwellingsList),
	                                    733, footerPos-18, "OVBUTN4.DEF");
	dwellDown->setOffset(6);
}

void CKingdomInterface::activateTab(size_t which)
{
	btnHeroes->block(which == 0);
	btnTowns->block(which == 1);
	tabArea->setActive(which);
}

void CKingdomInterface::townChanged(const CGTownInstance *town)
{
	if (CKingdTownList * townList = dynamic_cast<CKingdTownList*>(tabArea->getItem()))
		townList->townChanged(town);
}

void CKingdomInterface::updateGarrisons()
{
	if (CGarrisonHolder * garrison = dynamic_cast<CGarrisonHolder*>(tabArea->getItem()))
		garrison->updateGarrisons();
}

void CKingdomInterface::artifactAssembled(const ArtifactLocation& artLoc)
{
	if (CArtifactHolder * arts = dynamic_cast<CArtifactHolder*>(tabArea->getItem()))
		arts->artifactAssembled(artLoc);
}

void CKingdomInterface::artifactDisassembled(const ArtifactLocation& artLoc)
{
	if (CArtifactHolder * arts = dynamic_cast<CArtifactHolder*>(tabArea->getItem()))
		arts->artifactDisassembled(artLoc);
}

void CKingdomInterface::artifactMoved(const ArtifactLocation& artLoc, const ArtifactLocation& destLoc)
{
	if (CArtifactHolder * arts = dynamic_cast<CArtifactHolder*>(tabArea->getItem()))
		arts->artifactMoved(artLoc, destLoc);
}

void CKingdomInterface::artifactRemoved(const ArtifactLocation& artLoc)
{
	if (CArtifactHolder * arts = dynamic_cast<CArtifactHolder*>(tabArea->getItem()))
		arts->artifactRemoved(artLoc);
}

CKingdHeroList::CKingdHeroList(size_t maxSize)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	title = new CPicture("OVTITLE",16,0);
	title->colorize(LOCPLINT->playerID);
	heroLabel =   new CLabel(150, 10, FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->overview[0]);
	skillsLabel = new CLabel(500, 10, FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->overview[1]);

	ui32 townCount = LOCPLINT->cb->howManyHeroes(false);
	ui32 size = conf.go()->ac.overviewSize*116 + 19;
	heroes = new CListBox(boost::bind(&CKingdHeroList::createHeroItem, this, _1), boost::bind(&CKingdHeroList::destroyHeroItem, this, _1),
	                      Point(19,21), Point(0,116), maxSize, townCount, 0, 1, Rect(-19, -21, size, size) );
}

void CKingdHeroList::updateGarrisons()
{
	std::list<CIntObject*> list = heroes->getItems();
	BOOST_FOREACH(CIntObject* object, list)
	{
		if (CGarrisonHolder * garrison = dynamic_cast<CGarrisonHolder*>(object) )
			garrison->updateGarrisons();
	}
}

CIntObject* CKingdHeroList::createHeroItem(size_t index)
{
	ui32 picCount = conf.go()->ac.overviewPics;
	size_t heroesCount = LOCPLINT->cb->howManyHeroes(false);

	if (index < heroesCount)
	{
		CHeroItem * hero = new CHeroItem(LOCPLINT->cb->getHeroBySerial(index, false), &artsCommonPart);
		artsCommonPart.participants.insert(hero->heroArts);
		artSets.push_back(hero->heroArts);
		return hero;
	}
	else
	{
		return new CAnimImage("OVSLOT", (index-2) % picCount );
	}
}

void CKingdHeroList::destroyHeroItem(CIntObject *object)
{
	if (CHeroItem * hero = dynamic_cast<CHeroItem*>(object))
	{
		artSets.erase(std::find(artSets.begin(), artSets.end(), hero->heroArts));
		artsCommonPart.participants.erase(hero->heroArts);
	}
	delete object;
}

CKingdTownList::CKingdTownList(size_t maxSize)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	title = new CPicture("OVTITLE",16,0);
	title->colorize(LOCPLINT->playerID);
	townLabel   = new CLabel(146,10,FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->overview[3]);
	garrHeroLabel  = new CLabel(375,10,FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->overview[4]);
	visitHeroLabel = new CLabel(608,10,FONT_MEDIUM, CENTER, Colors::WHITE, CGI->generaltexth->overview[5]);

	ui32 townCount = LOCPLINT->cb->howManyTowns();
	ui32 size = conf.go()->ac.overviewSize*116 + 19;
	towns = new CListBox(boost::bind(&CKingdTownList::createTownItem, this, _1), CListBox::DestroyFunc(),
	                     Point(19,21), Point(0,116), maxSize, townCount, 0, 1, Rect(-19, -21, size, size) );
}

void CKingdTownList::townChanged(const CGTownInstance *town)
{
	std::list<CIntObject*> list = towns->getItems();
	BOOST_FOREACH(CIntObject* object, list)
	{
		CTownItem * townItem = dynamic_cast<CTownItem*>(object);
		if ( townItem && townItem->town == town)
			townItem->update();
	}
}

void CKingdTownList::updateGarrisons()
{
	std::list<CIntObject*> list = towns->getItems();
	BOOST_FOREACH(CIntObject* object, list)
	{
		if (CGarrisonHolder * garrison = dynamic_cast<CGarrisonHolder*>(object) )
			garrison->updateGarrisons();
	}
}

CIntObject* CKingdTownList::createTownItem(size_t index)
{
	ui32 picCount = conf.go()->ac.overviewPics;
	size_t townsCount = LOCPLINT->cb->howManyTowns();

	if (index < townsCount)
		return new CTownItem(LOCPLINT->cb->getTownBySerial(index));
	else
		return new CAnimImage("OVSLOT", (index-2) % picCount );
}

CTownItem::CTownItem(const CGTownInstance* Town):
	town(Town)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	background =  new CAnimImage("OVSLOT", 6);
	name = new CLabel(74, 8, FONT_SMALL, TOPLEFT, Colors::WHITE, town->name);

	income = new CLabel( 190, 60, FONT_SMALL, CENTER, Colors::WHITE, boost::lexical_cast<std::string>(town->dailyIncome()));
	hall = new CTownInfo( 69, 31, town, true);
	fort = new CTownInfo(111, 31, town, false);

	garr = new CGarrisonInt(313, 3, 4, Point(232,0),  NULL, Point(313,2), town->getUpperArmy(), town->visitingHero, true, true, true);
	heroes = new HeroSlots(town, Point(244,6), Point(475,6), garr, false);

	size_t iconIndex = town->town->clientInfo.icons[town->hasFort()][town->builded >= CGI->modh->settings.MAX_BUILDING_PER_TURN];

	picture = new CAnimImage("ITPT", iconIndex, 0, 5, 6);
	townArea = new LRClickableAreaOpenTown;
	townArea->pos = Rect(pos.x+5, pos.y+6, 58, 64);
	townArea->town = town;

	for (size_t i=0; i<town->creatures.size(); i++)
	{
		growth.push_back(new CCreaInfo(Point(401+37*i, 78), town, i, true, true));
		available.push_back(new CCreaInfo(Point(48+37*i, 78), town, i, true, false));
	}
}

void CTownItem::updateGarrisons()
{
	garr->selectSlot(nullptr);
	garr->setArmy(town->getUpperArmy(), 0);
	garr->setArmy(town->visitingHero, 1);
	garr->recreateSlots();
}

void CTownItem::update()
{
	std::string incomeVal = boost::lexical_cast<std::string>(town->dailyIncome());
	if (incomeVal != income->text)
		income->setTxt(incomeVal);

	heroes->update();

	for (size_t i=0; i<town->creatures.size(); i++)
	{
		growth[i]->update();
		available[i]->update();
	}
}

class ArtSlotsTab : public CIntObject
{
public:
	CAnimImage * background;
	std::vector<CArtPlace*> arts;

	ArtSlotsTab()
	{
		OBJ_CONSTRUCTION_CAPTURING_ALL;
		background = new CAnimImage("OVSLOT", 4);
		pos = background->pos;
		for (size_t i=0; i<9; i++)
			arts.push_back(new CArtPlace(Point(270+i*48, 65)));
	}
};

class BackpackTab : public CIntObject
{
public:
	CAnimImage * background;
	std::vector<CArtPlace*> arts;
	CAdventureMapButton *btnLeft;
	CAdventureMapButton *btnRight;

	BackpackTab()
	{
		OBJ_CONSTRUCTION_CAPTURING_ALL;
		background = new CAnimImage("OVSLOT", 5);
		pos = background->pos;
		btnLeft = new CAdventureMapButton(std::string(), std::string(), CFunctionList<void()>(), 269, 66, "HSBTNS3");
		btnRight = new CAdventureMapButton(std::string(), std::string(), CFunctionList<void()>(), 675, 66, "HSBTNS5");
		for (size_t i=0; i<8; i++)
			arts.push_back(new CArtPlace(Point(295+i*48, 65)));
	}
};

CHeroItem::CHeroItem(const CGHeroInstance* Hero, CArtifactsOfHero::SCommonPart * artsCommonPart):
	hero(Hero)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	artTabs.resize(3);
	ArtSlotsTab* arts1 = new ArtSlotsTab;
	ArtSlotsTab* arts2 = new ArtSlotsTab;
	BackpackTab* backpack = new BackpackTab;
	artTabs[0] = arts1;
	artTabs[1] = arts2;
	artTabs[2] = backpack;
	arts1->recActions = DISPOSE | SHARE_POS;
	arts2->recActions = DISPOSE | SHARE_POS;
	backpack->recActions = DISPOSE | SHARE_POS;

	name = new CLabel(75, 7, FONT_SMALL, TOPLEFT, Colors::WHITE, hero->name);

	std::vector<CArtPlace*> arts;
	arts.insert(arts.end(), arts1->arts.begin(), arts1->arts.end());
	arts.insert(arts.end(), arts2->arts.begin(), arts2->arts.end());

	heroArts = new CArtifactsOfHero(arts, backpack->arts, backpack->btnLeft, backpack->btnRight, false);
	heroArts->commonInfo = artsCommonPart;
	heroArts->setHero(hero);

	artsTabs = new CTabbedInt(boost::bind(&CHeroItem::onTabSelected, this, _1), boost::bind(&CHeroItem::onTabDeselected, this, _1));

	artButtons = new CHighlightableButtonsGroup(0);
	for (size_t it = 0; it<3; it++)
	{
		std::map<int,std::string> tooltip;
		tooltip[0] = CGI->generaltexth->overview[13+it];
		std::string overlay = CGI->generaltexth->overview[8+it];

		artButtons->addButton(tooltip, overlay, "OVBUTN3",364+it*112, 46, it);

		size_t begin = overlay.find('{');
		size_t end   = overlay.find('}', begin);
		overlay = overlay.substr(begin+1, end - begin);
		artButtons->buttons[it]->addTextOverlay(overlay, FONT_SMALL, Colors::YELLOW);
	}
	artButtons->onChange += boost::bind(&CTabbedInt::setActive, artsTabs, _1);
	artButtons->onChange += boost::bind(&CHeroItem::onArtChange, this, _1);
	artButtons->select(0,0);

	garr = new CGarrisonInt(6, 78, 4, Point(), NULL, Point(), hero, NULL, true, true);

	portrait = new CAnimImage("PortraitsLarge", hero->portrait, 0, 5, 6);
	heroArea = new CHeroArea(5, 6, hero);

	name = new CLabel(73, 7, FONT_SMALL, TOPLEFT, Colors::WHITE, hero->name);
	artsText = new CLabel(320, 55, FONT_SMALL, CENTER, Colors::WHITE, CGI->generaltexth->overview[2]);

	for (size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
		heroInfo.push_back(new InfoBox(Point(78+i*36, 26), InfoBox::POS_DOWN, InfoBox::SIZE_SMALL, 
		                   new InfoBoxHeroData(IInfoBoxData::HERO_PRIMARY_SKILL, hero, i)));

	for (size_t i=0; i<GameConstants::SKILL_PER_HERO; i++)
		heroInfo.push_back(new InfoBox(Point(410+i*36, 5), InfoBox::POS_NONE, InfoBox::SIZE_SMALL,
		                   new InfoBoxHeroData(IInfoBoxData::HERO_SECONDARY_SKILL, hero, i)));

	heroInfo.push_back(new InfoBox(Point(375, 5), InfoBox::POS_NONE, InfoBox::SIZE_SMALL,
	                   new InfoBoxHeroData(IInfoBoxData::HERO_SPECIAL, hero)));

	heroInfo.push_back(new InfoBox(Point(330, 5), InfoBox::POS_INSIDE, InfoBox::SIZE_SMALL,
	                   new InfoBoxHeroData(IInfoBoxData::HERO_EXPERIENCE, hero)));

	heroInfo.push_back(new InfoBox(Point(280, 5), InfoBox::POS_INSIDE, InfoBox::SIZE_SMALL,
	                   new InfoBoxHeroData(IInfoBoxData::HERO_MANA, hero)));

	morale = new MoraleLuckBox(true, Rect(225, 53, 30, 22), true);
	luck  = new MoraleLuckBox(false, Rect(225, 28, 30, 22), true);

	morale->set(hero);
	luck->set(hero);
}

CIntObject * CHeroItem::onTabSelected(size_t index)
{
	return artTabs[index];
}

void CHeroItem::onTabDeselected(CIntObject *object)
{
	addChild(object, false);
	object->recActions = DISPOSE | SHARE_POS;
}

void CHeroItem::onArtChange(int tabIndex)
{
	//redraw item after background change
	if (active)
		redraw();
}
