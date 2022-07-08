#pragma once

#include "main.h"

typedef struct MENUITEM {
	char* name;
	float minX;
	float minY;
	void(*Action)(void);
}MENUITEM;

typedef struct MENU {
	char* name;
	uint8_t currentSelectedMenuItem;
	uint8_t itemCount;
	MENUITEM** items;
}MENU;

void StartGameButtonAction(void);
void ControlsButtonAction(void);
void QuitButtonAction(void);