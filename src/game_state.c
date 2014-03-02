#include "game_state.h"
#include "in_game.h"

void initStateArray(gs_t ** s){
    int i;

    for(i = 0; i < NUM_STATES; i++){
        initInGameState(&((*s)[i]));
    }
}