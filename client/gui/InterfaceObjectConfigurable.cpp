/*
* InterfaceBuilder.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"

#include "InterfaceObjectConfigurable.h"

#include "../CGameInfo.h"
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../lib/CGeneralTextHandler.h"


InterfaceObjectConfigurable::InterfaceObjectConfigurable(const JsonNode & config, int used, Point offset):
	InterfaceObjectConfigurable(used, offset)
{
	init(config);
}

InterfaceObjectConfigurable::InterfaceObjectConfigurable(int used, Point offset):
	CIntObject(used, offset)
{
}

void InterfaceObjectConfigurable::addCallback(const std::string & callbackName, std::function<void(int)> callback)
{
	callbacks[callbackName] = callback;
}

void InterfaceObjectConfigurable::init(const JsonNode &config)
{
	OBJ_CONSTRUCTION;
	
	for(auto & item : config["variables"].Struct())
	{
		variables[item.first] = item.second;
	}
	
	int unnamedObjectId = 0;
	const std::string unnamedObjectPrefix = "__widget_";
	for(const auto & item : config["items"].Vector())
	{
		std::string name = item["name"].isNull()
						? unnamedObjectPrefix + std::to_string(unnamedObjectId++)
						: item["name"].String();
		widgets[name] = buildWidget(item);
	}
}

std::string InterfaceObjectConfigurable::readText(const JsonNode & config) const
{
	if(config.isNull())
		return "";
	
	if(config.isNumber())
	{
		return CGI->generaltexth->allTexts[config.Integer()];
	}
	
	const std::string delimiter = "/";
	std::string s = config.String();
	JsonNode translated = CGI->generaltexth->localizedTexts;
	for(size_t p = s.find(delimiter); p != std::string::npos; p = s.find(delimiter))
	{
		translated = translated[s.substr(0, p)];
		s.erase(0, p + delimiter.length());
	}
	if(s == config.String())
		return s;
	return translated[s].String();
}

Point InterfaceObjectConfigurable::readPosition(const JsonNode & config) const
{
	Point p;
	p.x = config["x"].Integer();
	p.y = config["y"].Integer();
	return p;
}

Rect InterfaceObjectConfigurable::readRect(const JsonNode & config) const
{
	Rect p;
	p.x = config["x"].Integer();
	p.y = config["y"].Integer();
	p.w = config["w"].Integer();
	p.h = config["h"].Integer();
	return p;
}

ETextAlignment InterfaceObjectConfigurable::readTextAlignment(const JsonNode & config) const
{
	if(!config.isNull())
	{
		if(config.String() == "center")
			return ETextAlignment::CENTER;
		if(config.String() == "left")
			return ETextAlignment::TOPLEFT;
		if(config.String() == "right")
			return ETextAlignment::BOTTOMRIGHT;
	}
	return ETextAlignment::CENTER;
}

SDL_Color InterfaceObjectConfigurable::readColor(const JsonNode & config) const
{
	if(!config.isNull())
	{
		if(config.String() == "yellow")
			return Colors::YELLOW;
		if(config.String() == "white")
			return Colors::WHITE;
		if(config.String() == "gold")
			return Colors::METALLIC_GOLD;
		if(config.String() == "green")
			return Colors::GREEN;
		if(config.String() == "orange")
			return Colors::ORANGE;
		if(config.String() == "bright-yellow")
			return Colors::BRIGHT_YELLOW;
	}
	return Colors::DEFAULT_KEY_COLOR;
	
}
EFonts InterfaceObjectConfigurable::readFont(const JsonNode & config) const
{
	if(!config.isNull())
	{
		if(config.String() == "big")
			return EFonts::FONT_BIG;
		if(config.String() == "medium")
			return EFonts::FONT_MEDIUM;
		if(config.String() == "small")
			return EFonts::FONT_SMALL;
		if(config.String() == "tiny")
			return EFonts::FONT_TINY;
	}
	return EFonts::FONT_TIMES;
}

std::pair<std::string, std::string> InterfaceObjectConfigurable::readHintText(const JsonNode & config) const
{
	std::pair<std::string, std::string> result;
	if(!config.isNull())
	{
		if(config.isNumber())
			return CGI->generaltexth->zelp[config.Integer()];
		
		if(config.getType() == JsonNode::JsonType::DATA_STRUCT)
		{
			result.first = readText(config["hover"]);
			result.second = readText(config["help"]);
			return result;
		}
		if(config.getType() == JsonNode::JsonType::DATA_STRING)
		{
			result.first = result.second = config.String();
		}
	}
	return result;
}

std::shared_ptr<CPicture> InterfaceObjectConfigurable::buildPicture(const JsonNode & config) const
{
	auto image = config["image"].String();
	auto position = readPosition(config["position"]);
	auto pic = std::make_shared<CPicture>(image, position.x, position.y);
	if(!config["visible"].isNull())
		pic->visible = config["visible"].Bool();
	return pic;
}

std::shared_ptr<CLabel> InterfaceObjectConfigurable::buildLabel(const JsonNode & config) const
{
	auto font = readFont(config["font"]);
	auto alignment = readTextAlignment(config["alignment"]);
	auto color = readColor(config["color"]);
	auto text = readText(config["text"]);
	auto position = readPosition(config["position"]);
	return std::make_shared<CLabel>(position.x, position.y, font, alignment, color, text);
}

std::shared_ptr<CToggleGroup> InterfaceObjectConfigurable::buildToggleGroup(const JsonNode & config) const
{
	auto position = readPosition(config["position"]);
	auto group = std::make_shared<CToggleGroup>(0);
	group->pos += position;
	if(!config["items"].isNull())
	{
		OBJ_CONSTRUCTION_TARGETED(group.get());
		int itemIdx = -1;
		for(const auto & item : config["items"].Vector())
		{
			itemIdx = item["index"].isNull() ? itemIdx + 1 : item["index"].Integer();
			group->addToggle(itemIdx, std::dynamic_pointer_cast<CToggleBase>(buildWidget(item)));
		}
	}
	if(!config["selected"].isNull())
		group->setSelected(config["selected"].Integer());
	if(!config["callback"].isNull())
		group->addCallback(callbacks.at(config["callback"].String()));
	return group;
}

std::shared_ptr<CToggleButton> InterfaceObjectConfigurable::buildToggleButton(const JsonNode & config) const
{
	auto position = readPosition(config["position"]);
	auto image = config["image"].String();
	auto zelp = readHintText(config["zelp"]);
	auto button = std::make_shared<CToggleButton>(position, image, zelp);
	if(!config["selected"].isNull())
		button->setSelected(config["selected"].Bool());
	if(!config["imageOrder"].isNull())
	{
		auto imgOrder = config["imageOrder"].Vector();
		assert(imgOrder.size() >= 4);
		button->setImageOrder(imgOrder[0].Integer(), imgOrder[1].Integer(), imgOrder[2].Integer(), imgOrder[3].Integer());
	}
	if(!config["callback"].isNull())
		button->addCallback(callbacks.at(config["callback"].String()));
	return button;
}

std::shared_ptr<CButton> InterfaceObjectConfigurable::buildButton(const JsonNode & config) const
{
	auto position = readPosition(config["position"]);
	auto image = config["image"].String();
	auto zelp = readHintText(config["zelp"]);
	auto button = std::make_shared<CButton>(position, image, zelp);
	if(!config["items"].isNull())
	{
		for(const auto & item : config["items"].Vector())
		{
			button->addOverlay(buildWidget(item));
		}
	}
	if(!config["callback"].isNull())
		button->addCallback(std::bind(callbacks.at(config["callback"].String()), 0));
	return button;
}

std::shared_ptr<CLabelGroup> InterfaceObjectConfigurable::buildLabelGroup(const JsonNode & config) const
{
	auto font = readFont(config["font"]);
	auto alignment = readTextAlignment(config["alignment"]);
	auto color = readColor(config["color"]);
	auto group = std::make_shared<CLabelGroup>(font, alignment, color);
	if(!config["items"].isNull())
	{
		for(const auto & item : config["items"].Vector())
		{
			auto position = readPosition(item["position"]);
			auto text = readText(item["text"]);
			group->add(position.x, position.y, text);
		}
	}
	return group;
}

std::shared_ptr<CSlider> InterfaceObjectConfigurable::buildSlider(const JsonNode & config) const
{
	auto position = readPosition(config["position"]);
	int length = config["size"].Integer();
	auto style = config["style"].String() == "brown" ? CSlider::BROWN : CSlider::BLUE;
	auto itemsVisible = config["itemsVisible"].Integer();
	auto itemsTotal = config["itemsTotal"].Integer();
	auto value = config["selected"].Integer();
	bool horizontal = config["orientation"].String() == "horizontal";
	return std::make_shared<CSlider>(position, length, callbacks.at(config["callback"].String()), itemsVisible, itemsTotal, value, horizontal, style);
}

std::shared_ptr<CAnimImage> InterfaceObjectConfigurable::buildImage(const JsonNode & config) const
{
	auto position = readPosition(config["position"]);
	auto image = config["image"].String();
	int group = config["group"].isNull() ? 0 : config["group"].Integer();
	int frame = config["frame"].isNull() ? 0 : config["frame"].Integer();
	return std::make_shared<CAnimImage>(image, frame, group, position.x, position.y);
}

std::shared_ptr<CFilledTexture> InterfaceObjectConfigurable::buildTexture(const JsonNode & config) const
{
	auto image = config["image"].String();
	auto rect = readRect(config["rect"]);
	return std::make_shared<CFilledTexture>(image, rect);
}

std::shared_ptr<CShowableAnim> InterfaceObjectConfigurable::buildAnimation(const JsonNode & config) const
{
	auto position = readPosition(config["position"]);
	auto image = config["image"].String();
	ui8 flags = 0;
	if(!config["repeat"].Bool())
		flags |= CShowableAnim::EFlags::PLAY_ONCE;
	
	int group = config["group"].isNull() ? 0 : config["group"].Integer();
	auto anim = std::make_shared<CShowableAnim>(position.x, position.y, image, flags, 4, group);
	if(!config["alpha"].isNull())
		anim->setAlpha(config["alpha"].Integer());
	if(!config["callback"].isNull())
		anim->callback = std::bind(callbacks.at(config["callback"].String()), 0);
	if(!config["frames"].isNull())
	{
		auto b = config["frames"]["start"].Integer();
		auto e = config["frames"]["end"].Integer();
		anim->set(group, b, e);
	}
	return anim;
}

std::shared_ptr<CIntObject> InterfaceObjectConfigurable::buildWidget(JsonNode config) const
{
	assert(!config.isNull());
	//overrides from variables
	for(auto & item : config["overrides"].Struct())
	{
		config[item.first] = variables[item.second.String()];
	}
	
	auto type = config["type"].String();
	if(type == "picture")
	{
		return buildPicture(config);
	}
	if(type == "image")
	{
		return buildImage(config);
	}
	if(type == "texture")
	{
		return buildTexture(config);
	}
	if(type == "animation")
	{
		return buildAnimation(config);
	}
	if(type == "label")
	{
		return buildLabel(config);
	}
	if(type == "toggleGroup")
	{
		return buildToggleGroup(config);
	}
	if(type == "toggleButton")
	{
		return buildToggleButton(config);
	}
	if(type == "button")
	{
		return buildButton(config);
	}
	if(type == "labelGroup")
	{
		return buildLabelGroup(config);
	}
	if(type == "slider")
	{
		return buildSlider(config);
	}
	if(type == "custom")
	{
		return const_cast<InterfaceObjectConfigurable*>(this)->buildCustomWidget(config);
	}
	return std::shared_ptr<CIntObject>(nullptr);
}

std::shared_ptr<CIntObject> InterfaceObjectConfigurable::buildCustomWidget(const JsonNode & config)
{
	return nullptr;
}
