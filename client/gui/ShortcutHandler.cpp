/*
 * ShortcutHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "ShortcutHandler.h"
#include "Shortcut.h"
#include <SDL_keycode.h>

std::vector<EShortcut> ShortcutHandler::translateKeycode(SDL_Keycode key) const
{
	static const std::multimap<SDL_Keycode, EShortcut> keyToShortcut = {
		{SDLK_RETURN,    EShortcut::GLOBAL_ACCEPT             },
		{SDLK_KP_ENTER,  EShortcut::GLOBAL_ACCEPT             },
		{SDLK_ESCAPE,    EShortcut::GLOBAL_CANCEL             },
		{SDLK_RETURN,    EShortcut::GLOBAL_RETURN             },
		{SDLK_KP_ENTER,  EShortcut::GLOBAL_RETURN             },
		{SDLK_ESCAPE,    EShortcut::GLOBAL_RETURN             },
		{SDLK_F4,        EShortcut::GLOBAL_FULLSCREEN         },
		{SDLK_BACKSPACE, EShortcut::GLOBAL_BACKSPACE          },
		{SDLK_TAB,       EShortcut::GLOBAL_MOVE_FOCUS         },
		{SDLK_o,         EShortcut::GLOBAL_OPTIONS            },
		{SDLK_LEFT,      EShortcut::MOVE_LEFT                 },
		{SDLK_RIGHT,     EShortcut::MOVE_RIGHT                },
		{SDLK_UP,        EShortcut::MOVE_UP                   },
		{SDLK_DOWN,      EShortcut::MOVE_DOWN                 },
		{SDLK_HOME,      EShortcut::MOVE_FIRST                },
		{SDLK_END,       EShortcut::MOVE_LAST                 },
		{SDLK_PAGEUP,    EShortcut::MOVE_PAGE_UP              },
		{SDLK_PAGEDOWN,  EShortcut::MOVE_PAGE_DOWN            },
		{SDLK_1,         EShortcut::SELECT_INDEX_1            },
		{SDLK_2,         EShortcut::SELECT_INDEX_2            },
		{SDLK_3,         EShortcut::SELECT_INDEX_3            },
		{SDLK_4,         EShortcut::SELECT_INDEX_4            },
		{SDLK_5,         EShortcut::SELECT_INDEX_5            },
		{SDLK_6,         EShortcut::SELECT_INDEX_6            },
		{SDLK_7,         EShortcut::SELECT_INDEX_7            },
		{SDLK_8,         EShortcut::SELECT_INDEX_8            },
		{SDLK_n,         EShortcut::MAIN_MENU_NEW_GAME        },
		{SDLK_l,         EShortcut::MAIN_MENU_LOAD_GAME       },
		{SDLK_h,         EShortcut::MAIN_MENU_HIGH_SCORES     },
		{SDLK_c,         EShortcut::MAIN_MENU_CREDITS         },
		{SDLK_q,         EShortcut::MAIN_MENU_QUIT            },
		{SDLK_b,         EShortcut::MAIN_MENU_BACK            },
		{SDLK_s,         EShortcut::MAIN_MENU_SINGLEPLAYER    },
		{SDLK_m,         EShortcut::MAIN_MENU_MULTIPLAYER     },
		{SDLK_c,         EShortcut::MAIN_MENU_CAMPAIGN        },
		{SDLK_t,         EShortcut::MAIN_MENU_TUTORIAL        },
		{SDLK_s,         EShortcut::MAIN_MENU_CAMPAIGN_SOD    },
		{SDLK_r,         EShortcut::MAIN_MENU_CAMPAIGN_ROE    },
		{SDLK_a,         EShortcut::MAIN_MENU_CAMPAIGN_AB     },
		{SDLK_c,         EShortcut::MAIN_MENU_CAMPAIGN_CUSTOM },
		{SDLK_b,         EShortcut::LOBBY_BEGIN_GAME          },
		{SDLK_RETURN,    EShortcut::LOBBY_BEGIN_GAME          },
		{SDLK_KP_ENTER,  EShortcut::LOBBY_BEGIN_GAME          },
		{SDLK_l,         EShortcut::LOBBY_LOAD_GAME           },
		{SDLK_RETURN,    EShortcut::LOBBY_LOAD_GAME           },
		{SDLK_KP_ENTER,  EShortcut::LOBBY_LOAD_GAME           },
		{SDLK_s,         EShortcut::LOBBY_SAVE_GAME           },
		{SDLK_r,         EShortcut::LOBBY_RANDOM_MAP          },
		{SDLK_h,         EShortcut::LOBBY_HIDE_CHAT           },
		{SDLK_a,         EShortcut::LOBBY_ADDITIONAL_OPTIONS  },
		{SDLK_s,         EShortcut::LOBBY_SELECT_SCENARIO     },
		{SDLK_e,         EShortcut::GAME_END_TURN             },
		{SDLK_l,         EShortcut::GAME_LOAD_GAME            },
		{SDLK_s,         EShortcut::GAME_SAVE_GAME            },
		{SDLK_r,         EShortcut::GAME_RESTART_GAME         },
		{SDLK_m,         EShortcut::GAME_TO_MAIN_MENU         },
		{SDLK_q,         EShortcut::GAME_QUIT_GAME            },
		{SDLK_t,         EShortcut::GAME_OPEN_MARKETPLACE     },
		{SDLK_g,         EShortcut::GAME_OPEN_THIEVES_GUILD   },
		{SDLK_TAB,       EShortcut::GAME_ACTIVATE_CONSOLE     },
		{SDLK_o,         EShortcut::ADVENTURE_GAME_OPTIONS    },
		{SDLK_F6,        EShortcut::ADVENTURE_TOGGLE_GRID     },
		{SDLK_z,         EShortcut::ADVENTURE_TOGGLE_SLEEP    },
		{SDLK_w,         EShortcut::ADVENTURE_TOGGLE_SLEEP    },
		{SDLK_m,         EShortcut::ADVENTURE_MOVE_HERO       },
		{SDLK_SPACE,     EShortcut::ADVENTURE_VISIT_OBJECT    },
		{SDLK_KP_1,      EShortcut::ADVENTURE_MOVE_HERO_SW    },
		{SDLK_KP_2,      EShortcut::ADVENTURE_MOVE_HERO_SS    },
		{SDLK_KP_3,      EShortcut::ADVENTURE_MOVE_HERO_SE    },
		{SDLK_KP_4,      EShortcut::ADVENTURE_MOVE_HERO_WW    },
		{SDLK_KP_6,      EShortcut::ADVENTURE_MOVE_HERO_EE    },
		{SDLK_KP_7,      EShortcut::ADVENTURE_MOVE_HERO_NW    },
		{SDLK_KP_8,      EShortcut::ADVENTURE_MOVE_HERO_NN    },
		{SDLK_KP_9,      EShortcut::ADVENTURE_MOVE_HERO_NE    },
		{SDLK_DOWN,      EShortcut::ADVENTURE_MOVE_HERO_SS    },
		{SDLK_LEFT,      EShortcut::ADVENTURE_MOVE_HERO_WW    },
		{SDLK_RIGHT,     EShortcut::ADVENTURE_MOVE_HERO_EE    },
		{SDLK_UP,        EShortcut::ADVENTURE_MOVE_HERO_NN    },
		{SDLK_RETURN,    EShortcut::ADVENTURE_VIEW_SELECTED   },
		{SDLK_KP_ENTER,  EShortcut::ADVENTURE_VIEW_SELECTED   },
 //		{SDLK_,          EShortcut::ADVENTURE_NEXT_OBJECT     },
		{SDLK_t,         EShortcut::ADVENTURE_NEXT_TOWN       },
		{SDLK_h,         EShortcut::ADVENTURE_NEXT_HERO       },
 //		{SDLK_,          EShortcut::ADVENTURE_FIRST_TOWN      },
  //		{SDLK_,          EShortcut::ADVENTURE_FIRST_HERO      },
		{SDLK_i,         EShortcut::ADVENTURE_VIEW_SCENARIO   },
		{SDLK_d,         EShortcut::ADVENTURE_DIG_GRAIL       },
		{SDLK_p,         EShortcut::ADVENTURE_VIEW_PUZZLE     },
		{SDLK_v,         EShortcut::ADVENTURE_VIEW_WORLD      },
		{SDLK_u,         EShortcut::ADVENTURE_TOGGLE_MAP_LEVEL},
		{SDLK_k,         EShortcut::ADVENTURE_KINGDOM_OVERVIEW},
		{SDLK_q,         EShortcut::ADVENTURE_QUEST_LOG       },
		{SDLK_c,         EShortcut::ADVENTURE_CAST_SPELL      },
		{SDLK_e,         EShortcut::ADVENTURE_END_TURN        },
		{SDLK_g,         EShortcut::ADVENTURE_THIEVES_GUILD   },
		{SDLK_q,         EShortcut::BATTLE_TOGGLE_QUEUE       },
		{SDLK_c,         EShortcut::BATTLE_USE_CREATURE_SPELL },
		{SDLK_s,         EShortcut::BATTLE_SURRENDER          },
		{SDLK_r,         EShortcut::BATTLE_RETREAT            },
		{SDLK_a,         EShortcut::BATTLE_AUTOCOMBAT         },
		{SDLK_c,         EShortcut::BATTLE_CAST_SPELL         },
		{SDLK_w,         EShortcut::BATTLE_WAIT               },
		{SDLK_d,         EShortcut::BATTLE_DEFEND             },
		{SDLK_SPACE,     EShortcut::BATTLE_DEFEND             },
		{SDLK_UP,        EShortcut::BATTLE_CONSOLE_UP         },
		{SDLK_DOWN,      EShortcut::BATTLE_CONSOLE_DOWN       },
		{SDLK_SPACE,     EShortcut::BATTLE_TACTICS_NEXT       },
		{SDLK_RETURN,    EShortcut::BATTLE_TACTICS_END        },
		{SDLK_KP_ENTER,  EShortcut::BATTLE_TACTICS_END        },
		{SDLK_s,         EShortcut::BATTLE_SELECT_ACTION      },
		{SDLK_t,         EShortcut::TOWN_OPEN_TAVERN          },
		{SDLK_SPACE,     EShortcut::TOWN_SWAP_ARMIES          },
		{SDLK_END,       EShortcut::RECRUITMENT_MAX           },
		{SDLK_HOME,      EShortcut::RECRUITMENT_MIN           },
		{SDLK_u,         EShortcut::RECRUITMENT_UPGRADE       },
		{SDLK_a,         EShortcut::RECRUITMENT_UPGRADE_ALL   },
		{SDLK_u,         EShortcut::RECRUITMENT_UPGRADE_ALL   },
		{SDLK_h,         EShortcut::KINGDOM_HEROES_TAB        },
		{SDLK_t,         EShortcut::KINGDOM_TOWNS_TAB         },
		{SDLK_d,         EShortcut::HERO_DISMISS              },
		{SDLK_c,         EShortcut::HERO_COMMANDER            },
		{SDLK_l,         EShortcut::HERO_LOOSE_FORMATION      },
		{SDLK_t,         EShortcut::HERO_TIGHT_FORMATION      },
		{SDLK_b,         EShortcut::HERO_TOGGLE_TACTICS       },
		{SDLK_a,         EShortcut::SPELLBOOK_TAB_ADVENTURE   },
		{SDLK_c,         EShortcut::SPELLBOOK_TAB_COMBAT      }
	};

	auto range = keyToShortcut.equal_range(key);

	// FIXME: some code expects calls to keyPressed / captureThisKey even without defined hotkeys
	if (range.first == range.second)
		return {EShortcut::NONE};

	std::vector<EShortcut> result;

	for (auto it = range.first; it != range.second; ++it)
		result.push_back(it->second);

	return result;
}

EShortcut ShortcutHandler::findShortcut(const std::string & identifier ) const
{
	static const std::map<std::string, EShortcut> shortcutNames = {
		{"globalAccept",             EShortcut::GLOBAL_ACCEPT             },
		{"globalCancel",             EShortcut::GLOBAL_CANCEL             },
		{"globalReturn",             EShortcut::GLOBAL_RETURN             },
		{"globalFullscreen",         EShortcut::GLOBAL_FULLSCREEN         },
		{"globalOptions",            EShortcut::GLOBAL_OPTIONS            },
		{"globalBackspace",          EShortcut::GLOBAL_BACKSPACE          },
		{"globalMoveFocus",          EShortcut::GLOBAL_MOVE_FOCUS         },
		{"moveLeft",                 EShortcut::MOVE_LEFT                 },
		{"moveRight",                EShortcut::MOVE_RIGHT                },
		{"moveUp",                   EShortcut::MOVE_UP                   },
		{"moveDown",                 EShortcut::MOVE_DOWN                 },
		{"moveFirst",                EShortcut::MOVE_FIRST                },
		{"moveLast",                 EShortcut::MOVE_LAST                 },
		{"movePageUp",               EShortcut::MOVE_PAGE_UP              },
		{"movePageDown",             EShortcut::MOVE_PAGE_DOWN            },
		{"selectIndex1",             EShortcut::SELECT_INDEX_1            },
		{"selectIndex2",             EShortcut::SELECT_INDEX_2            },
		{"selectIndex3",             EShortcut::SELECT_INDEX_3            },
		{"selectIndex4",             EShortcut::SELECT_INDEX_4            },
		{"selectIndex5",             EShortcut::SELECT_INDEX_5            },
		{"selectIndex6",             EShortcut::SELECT_INDEX_6            },
		{"selectIndex7",             EShortcut::SELECT_INDEX_7            },
		{"selectIndex8",             EShortcut::SELECT_INDEX_8            },
		{"mainMenuNewGame",          EShortcut::MAIN_MENU_NEW_GAME        },
		{"mainMenuLoadGame",         EShortcut::MAIN_MENU_LOAD_GAME       },
		{"mainMenuHighScores",       EShortcut::MAIN_MENU_HIGH_SCORES     },
		{"mainMenuCredits",          EShortcut::MAIN_MENU_CREDITS         },
		{"mainMenuQuit",             EShortcut::MAIN_MENU_QUIT            },
		{"mainMenuBack",             EShortcut::MAIN_MENU_BACK            },
		{"mainMenuSingleplayer",     EShortcut::MAIN_MENU_SINGLEPLAYER    },
		{"mainMenuMultiplayer",      EShortcut::MAIN_MENU_MULTIPLAYER     },
		{"mainMenuCampaign",         EShortcut::MAIN_MENU_CAMPAIGN        },
		{"mainMenuTutorial",         EShortcut::MAIN_MENU_TUTORIAL        },
		{"mainMenuCampaignSod",      EShortcut::MAIN_MENU_CAMPAIGN_SOD    },
		{"mainMenuCampaignRoe",      EShortcut::MAIN_MENU_CAMPAIGN_ROE    },
		{"mainMenuCampaignAb",       EShortcut::MAIN_MENU_CAMPAIGN_AB     },
		{"mainMenuCampaignCustom",   EShortcut::MAIN_MENU_CAMPAIGN_CUSTOM },
		{"lobbyBeginGame",           EShortcut::LOBBY_BEGIN_GAME          },
		{"lobbyLoadGame",            EShortcut::LOBBY_LOAD_GAME           },
		{"lobbySaveGame",            EShortcut::LOBBY_SAVE_GAME           },
		{"lobbyRandomMap",           EShortcut::LOBBY_RANDOM_MAP          },
		{"lobbyHideChat",            EShortcut::LOBBY_HIDE_CHAT           },
		{"lobbyAdditionalOptions",   EShortcut::LOBBY_ADDITIONAL_OPTIONS  },
		{"lobbySelectScenario",      EShortcut::LOBBY_SELECT_SCENARIO     },
		{"gameEndTurn",              EShortcut::GAME_END_TURN             },
		{"gameLoadGame",             EShortcut::GAME_LOAD_GAME            },
		{"gameSaveGame",             EShortcut::GAME_SAVE_GAME            },
		{"gameRestartGame",          EShortcut::GAME_RESTART_GAME         },
		{"gameMainMenu",             EShortcut::GAME_TO_MAIN_MENU         },
		{"gameQuitGame",             EShortcut::GAME_QUIT_GAME            },
		{"gameOpenMarketplace",      EShortcut::GAME_OPEN_MARKETPLACE     },
		{"gameOpenThievesGuild",     EShortcut::GAME_OPEN_THIEVES_GUILD   },
		{"gameActivateConsole",      EShortcut::GAME_ACTIVATE_CONSOLE     },
		{"adventureGameOptions",     EShortcut::ADVENTURE_GAME_OPTIONS    },
		{"adventureToggleGrid",      EShortcut::ADVENTURE_TOGGLE_GRID     },
		{"adventureToggleSleep",     EShortcut::ADVENTURE_TOGGLE_SLEEP    },
		{"adventureMoveHero",        EShortcut::ADVENTURE_MOVE_HERO       },
		{"adventureVisitObject",     EShortcut::ADVENTURE_VISIT_OBJECT    },
		{"adventureMoveHeroSW",      EShortcut::ADVENTURE_MOVE_HERO_SW    },
		{"adventureMoveHeroSS",      EShortcut::ADVENTURE_MOVE_HERO_SS    },
		{"adventureMoveHeroSE",      EShortcut::ADVENTURE_MOVE_HERO_SE    },
		{"adventureMoveHeroWW",      EShortcut::ADVENTURE_MOVE_HERO_WW    },
		{"adventureMoveHeroEE",      EShortcut::ADVENTURE_MOVE_HERO_EE    },
		{"adventureMoveHeroNW",      EShortcut::ADVENTURE_MOVE_HERO_NW    },
		{"adventureMoveHeroNN",      EShortcut::ADVENTURE_MOVE_HERO_NN    },
		{"adventureMoveHeroNE",      EShortcut::ADVENTURE_MOVE_HERO_NE    },
		{"adventureViewSelected",    EShortcut::ADVENTURE_VIEW_SELECTED   },
		{"adventureNextObject",      EShortcut::ADVENTURE_NEXT_OBJECT     },
		{"adventureNextTown",        EShortcut::ADVENTURE_NEXT_TOWN       },
		{"adventureNextHero",        EShortcut::ADVENTURE_NEXT_HERO       },
		{"adventureFirstTown",       EShortcut::ADVENTURE_FIRST_TOWN      },
		{"adventureFirstHero",       EShortcut::ADVENTURE_FIRST_HERO      },
		{"adventureViewScenario",    EShortcut::ADVENTURE_VIEW_SCENARIO   },
		{"adventureDigGrail",        EShortcut::ADVENTURE_DIG_GRAIL       },
		{"adventureViewPuzzle",      EShortcut::ADVENTURE_VIEW_PUZZLE     },
		{"adventureViewWorld",       EShortcut::ADVENTURE_VIEW_WORLD      },
		{"adventureToggleMapLevel",  EShortcut::ADVENTURE_TOGGLE_MAP_LEVEL},
		{"adventureKingdomOverview", EShortcut::ADVENTURE_KINGDOM_OVERVIEW},
		{"adventureQuestLog",        EShortcut::ADVENTURE_QUEST_LOG       },
		{"adventureCastSpell",       EShortcut::ADVENTURE_CAST_SPELL      },
		{"adventureEndTurn",         EShortcut::ADVENTURE_END_TURN        },
		{"adventureThievesGuild",    EShortcut::ADVENTURE_THIEVES_GUILD   },
		{"battleToggleQueue",        EShortcut::BATTLE_TOGGLE_QUEUE       },
		{"battleUseCreatureSpell",   EShortcut::BATTLE_USE_CREATURE_SPELL },
		{"battleSurrender",          EShortcut::BATTLE_SURRENDER          },
		{"battleRetreat",            EShortcut::BATTLE_RETREAT            },
		{"battleAutocombat",         EShortcut::BATTLE_AUTOCOMBAT         },
		{"battleCastSpell",          EShortcut::BATTLE_CAST_SPELL         },
		{"battleWait",               EShortcut::BATTLE_WAIT               },
		{"battleDefend",             EShortcut::BATTLE_DEFEND             },
		{"battleConsoleUp",          EShortcut::BATTLE_CONSOLE_UP         },
		{"battleConsoleDown",        EShortcut::BATTLE_CONSOLE_DOWN       },
		{"battleTacticsNext",        EShortcut::BATTLE_TACTICS_NEXT       },
		{"battleTacticsEnd",         EShortcut::BATTLE_TACTICS_END        },
		{"battleSelectAction",       EShortcut::BATTLE_SELECT_ACTION      },
		{"townOpenTavern",           EShortcut::TOWN_OPEN_TAVERN          },
		{"townSwapArmies",           EShortcut::TOWN_SWAP_ARMIES          },
		{"recruitmentMax",           EShortcut::RECRUITMENT_MAX           },
		{"recruitmentMin",           EShortcut::RECRUITMENT_MIN           },
		{"recruitmentUpgrade",       EShortcut::RECRUITMENT_UPGRADE       },
		{"recruitmentUpgradeAll",    EShortcut::RECRUITMENT_UPGRADE_ALL   },
		{"kingdomHeroesTab",         EShortcut::KINGDOM_HEROES_TAB        },
		{"kingdomTownsTab",          EShortcut::KINGDOM_TOWNS_TAB         },
		{"heroDismiss",              EShortcut::HERO_DISMISS              },
		{"heroCommander",            EShortcut::HERO_COMMANDER            },
		{"heroLooseFormation",       EShortcut::HERO_LOOSE_FORMATION      },
		{"heroTightFormation",       EShortcut::HERO_TIGHT_FORMATION      },
		{"heroToggleTactics",        EShortcut::HERO_TOGGLE_TACTICS       },
		{"spellbookTabAdventure",    EShortcut::SPELLBOOK_TAB_ADVENTURE   },
		{"spellbookTabCombat",       EShortcut::SPELLBOOK_TAB_COMBAT      }
	};

	if (shortcutNames.count(identifier))
		return shortcutNames.at(identifier);
	return EShortcut::NONE;
}


















































































