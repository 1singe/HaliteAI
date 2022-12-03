#include "spawnManager.hpp"

SpawnManager::SpawnManager(const hlt::Game& r_game, const ai::DropoffManager& kr_dropoffManager )
	: mr_game(r_game), mkr_dropoffManager(kr_dropoffManager)
{}

bool SpawnManager::canSpawn() const {
	//ne pas construire si pas assez de halite		
	if (mr_game.me->halite < hlt::constants::SHIP_COST) {
		return false;
	}

	//ne pas constuire si trop de tours écoulés
	if (mr_game.turn_number > hlt::constants::MAX_TURNS * k_maxTurnForSpawningRatio) {
		return false;
	}

	//ne pas construire si il y a un allié sur le spawneur
	if (mr_game.game_map->at(mr_game.me->shipyard)->is_occupied()) {
		if (mr_game.game_map->at(mr_game.me->shipyard)->ship->owner == mr_game.me->id) {
			return false;
		}
	}

	//ne pas construire si il y a un plan de construction de dropoff en route et que pas assez de halite
	ai::DropoffPlan* const plan = mkr_dropoffManager.getDropoffPlan();
	if (plan != nullptr) {
		if (hlt::constants::DROPOFF_COST + hlt::constants::SHIP_COST - mr_game.game_map->at(plan->position)->halite > mr_game.me->halite) {
			return false;
		}
	}

	//ne pas etre celui avec le moins de vaisseaux && ne pas avoir bcp plus de vaisseaux que le 2e player
	int maxShips = 0;
	size_t myShipsCount = mr_game.me->ships.size();
	for (const std::shared_ptr<hlt::Player>& kr_player : mr_game.players) {
		size_t numberOfShips = kr_player->ships.size();
		if (myShipsCount < numberOfShips) {
			return true;
		}
		else if (numberOfShips > maxShips) {
			maxShips = numberOfShips;
		}
	}
	if (myShipsCount > maxShips + k_maxShipAdvantage) {
		return false;
	}

	return true;
}
