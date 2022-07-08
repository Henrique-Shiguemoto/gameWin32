#pragma once

#include<windows.h>

#include "main.h"

extern GAMESTATE g_GameState;

void StartGameButtonAction(void) {

}

void ControlsButtonAction(void) {

}

void QuitButtonAction(void) {
	g_GameState = FALSE;
}