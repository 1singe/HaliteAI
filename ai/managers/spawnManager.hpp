#pragma once

#include "../../hlt/game.hpp"
#include "../../hlt/constants.hpp"
#include "dropoffManager.hpp"

#include <map>
#include <vector>

class SpawnManager {
	
public:
	
	SpawnManager(const hlt::Game& r_game, const ai::DropoffManager& kr_dropoffManager );

	//retourne si un vaisseau doit etre construit ce tour
	bool canSpawn() const ;

private:
	const hlt::Game& mr_game;
	const ai::DropoffManager& mkr_dropoffManager;

	const float k_maxTurnForSpawningRatio = 0.63f;
	const int k_maxShipAdvantage = 10;

};