#include "dropoffManager.hpp"

ai::DropoffManager::DropoffManager(const hlt::Game& kr_game) : mr_game(kr_game){
	mp_currentPlan = nullptr;
}

ai::DropoffManager::~DropoffManager() {
	deletePlan();
}

bool ai::DropoffManager::shouldBuildADropoff() {
	//ne pas construire si trop tard dans le jeux
	if (mr_game.turn_number > k_maxTurnForBuildingDroppoffRatio*hlt::constants::MAX_TURNS) {
		return false;
	}

	//ne pas construire si trop de dropoff
	if (mr_game.me->dropoffs.size() > k_maxDropoffCount) {
		return false;
	}

	//ne pas construire si pas assez de bateaux ou pas assez de tours écoulés
	if (mr_game.me->ships.size() < k_shipsNeededToBuild[mr_game.me->dropoffs.size()]
		&& mr_game.turn_number < k_turnNeededToBuild[mr_game.me->dropoffs.size()]) {
		return false;
	}

	//trouver la meilleur case pour construire
	DropoffPlan* potentialPlan = new DropoffPlan();
	for (int x = 0; x < mr_game.game_map->width; x++) {
		for (int y = 0; y < mr_game.game_map->height; y++) {
			hlt::MapCell* const kp_cell = mr_game.game_map->at(hlt::Position(x, y));

			//pas déjà de structure dessus
			if (kp_cell->has_structure()) {
				continue;
			}

			bool shouldContinue = false;
			//pas d'ennemi autour
			for (hlt::Position& kr_neighborCell : kp_cell->position.get_surrounding_cardinals()) {
				if (mr_game.game_map->at(kr_neighborCell)->is_occupied()) {
					if (mr_game.game_map->at(kr_neighborCell)->ship->owner != mr_game.me->id) {
						shouldContinue = true;
						break;
					}
				}
				//ni de structure ennemi
				if (mr_game.game_map->at(kr_neighborCell)->has_structure()) {
					if (mr_game.game_map->at(kr_neighborCell)->structure->owner != mr_game.me->id) {
						shouldContinue = true;
						break;
					}
				}
			}
			if (shouldContinue){
				continue;
			}

			//ni dessus
			if (mr_game.game_map->at(kp_cell->position)->is_occupied()) {
				if (mr_game.game_map->at(kp_cell->position)->ship->owner != mr_game.me->id) {
					continue;
				}
			}

			//pas de dropoff trop proche
			shouldContinue = false;
			for (const std::pair<hlt::EntityId, std::shared_ptr<hlt::Dropoff>>& kr_dropoff : mr_game.me->dropoffs) {
				if (mr_game.game_map->calculate_distance(kp_cell->position, kr_dropoff.second->position) < k_distanceBetweenDropoff) {
					shouldContinue = true;
					break;
				}
			}
			if (shouldContinue) {
				continue;
			}

			//pas trop proche du shipyard
			if (mr_game.game_map->calculate_distance(kp_cell->position, mr_game.me->shipyard->position) < k_distanceBetweenDropoff) {
				continue;
			}

			//pas de dropoff s'il y a pas de vaisseaux aliés au tour
			{
				bool isThereAShipAround = false;
				for (int localX = -k_friendShipRadius; localX <= k_friendShipRadius; localX++) {
					if (isThereAShipAround) {
						break;
					}
					for (int localY = -k_friendShipRadius; localY <= k_friendShipRadius; localY++) {
						hlt::MapCell* kr_cell = mr_game.game_map->at(hlt::Position(x + localX, y + localY));
						if (kr_cell->is_occupied()) {
							if (kr_cell->ship->owner == mr_game.me->id) {
								isThereAShipAround = true;
								break;
							}
						}
					}
				}
				if (isThereAShipAround == false) {
					continue;
				}
			}

			//calcule le nombre de halite autour
			{
				int haliteSum = 0;
				for (int localX = -k_dropoffSumRadius; localX <= k_dropoffSumRadius; localX++) {
					for (int localY = -k_dropoffSumRadius; localY <= k_dropoffSumRadius; localY++) {
						haliteSum += (mr_game.game_map->at(hlt::Position(x + localX, y + localY)))->halite;
					}
				}
				if (haliteSum > potentialPlan->haliteAt) {
					potentialPlan->haliteAt = haliteSum;
					potentialPlan->position.x = x;
					potentialPlan->position.y = y;
				}
			}
		}
	}

	if (potentialPlan->haliteAt > 0) {
		//regarder si il y a deja un plan
		if (mp_currentPlan != nullptr && mr_game.game_map->at(mp_currentPlan->position)->has_structure()) {
			deletePlan();
		}

		if (mp_currentPlan != nullptr) {

			//regade si le constructeur du plan d'avant est mort
			if (mr_game.me->ships.find(mp_currentPlan->builder) == mr_game.me->ships.end())
			{
				//lui assigner un nouveau constructeur && calculer le coup de construction
				std::pair<std::shared_ptr<hlt::Ship>, int> newBuilder = getBuilder(mp_currentPlan->position);
				if (newBuilder.first != nullptr) {
					mp_currentPlan->builder = newBuilder.first->id;
					mp_currentPlan->estimatedCost = hlt::constants::DROPOFF_COST - mr_game.game_map->at(mp_currentPlan->position)->halite - newBuilder.second;
				}
				else {
					//si pas de builder trouvé, l'abandonner
					deletePlan();
				}
			}

			//calculer le coup du nouveau plan
			std::pair<std::shared_ptr<hlt::Ship>, int> newBuilder = getBuilder(potentialPlan->position);
			if (newBuilder.first != nullptr) {
				potentialPlan->builder = newBuilder.first->id;
				potentialPlan->estimatedCost = hlt::constants::DROPOFF_COST - mr_game.game_map->at(potentialPlan->position)->halite - newBuilder.second;
			}

			//si le plan d'avant est tjrs là
			if (mp_currentPlan != nullptr) {
				//comparer les deux plan, et garder le meilleur
				if (potentialPlan->estimatedCost < mp_currentPlan->estimatedCost) {
					mp_currentPlan = potentialPlan;
				}
				else {
					delete potentialPlan;
				}
			}
			else {
				//prend le nouveau
				mp_currentPlan = potentialPlan;
			}
		}
		// si il n'y avait pas de plan
		else {
			//calculer le coup du nouveau plan
			std::pair<std::shared_ptr<hlt::Ship>, int> newBuilder = getBuilder(potentialPlan->position);
			if (newBuilder.first != nullptr) {
				potentialPlan->builder = newBuilder.first->id;
				potentialPlan->estimatedCost = hlt::constants::DROPOFF_COST - mr_game.game_map->at(potentialPlan->position)->halite - newBuilder.second;
			}
			else {
				return false;
			}

			mp_currentPlan = potentialPlan;
		}
		return true;
	}
	else {
		delete potentialPlan;
		return false;
	}

}

void ai::DropoffManager::deletePlan() {
	if (mp_currentPlan != nullptr) {
		delete mp_currentPlan;
	}
	mp_currentPlan = nullptr;
}

std::pair<std::shared_ptr<hlt::Ship>, int> ai::DropoffManager::getBuilder(hlt::Position dropoffPosition) const {
	std::shared_ptr<hlt::Ship> bestBuilder = nullptr;
	int biggestHaliteBrought = 0;

	for (int localX = -k_friendShipRadius; localX <= k_friendShipRadius; localX++) {
		for (int localY = -k_friendShipRadius; localY <= k_friendShipRadius; localY++) {
			hlt::MapCell* kr_cell = mr_game.game_map->at(hlt::Position(dropoffPosition.x + localX, dropoffPosition.y + localY));
			if (kr_cell->is_occupied()) {
				if (kr_cell->ship->owner == mr_game.me->id) {

					int estimatedHalite = kr_cell->ship->halite;
					int distance = mr_game.game_map->calculate_distance(dropoffPosition, kr_cell->ship->position);
					estimatedHalite -= distance * constants::AVERAGE_HALITE_PER_CELL;

					if (estimatedHalite > biggestHaliteBrought || biggestHaliteBrought==0) {
						biggestHaliteBrought = estimatedHalite;
						bestBuilder = kr_cell->ship;
					}
				}
			}
		}
	}
	return std::make_pair(bestBuilder, biggestHaliteBrought);
}

ai::DropoffPlan* const ai::DropoffManager::getDropoffPlan() const {
	return mp_currentPlan;
}
