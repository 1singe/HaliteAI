#include "../ai/blackBoard.hpp"

ai::BlackBoard* ai::BlackBoard::I() {
	if (sp_instance != nullptr)
		return sp_instance;
	else
		throw("Blackboard's singleton has not been initialized");
}


ai::BlackBoard::BlackBoard(const hlt::Game& kr_game, HaliteToAstarLinker& r_hltToAstar) :
	mkr_game(kr_game), mr_hltToAstar(r_hltToAstar)
{
	int size = ai::constants::MAP_SIZE;
	pp_flattenedHaliteScoreArray = new float* [size];
	pp_distanceToDropOffsArray = new float* [size];
	pp_inspirationArray = new float* [size];
	pp_disponibilityArray = new bool* [size];
	pp_targetArray = new bool* [size];
	for (int x = 0; x < size; x++) {
		pp_flattenedHaliteScoreArray[x] = new float[size]();
		pp_distanceToDropOffsArray[x] = new float[size]();
		pp_inspirationArray[x] = new float[size]();
		pp_disponibilityArray[x] = new bool[size]();
		pp_targetArray[x] = new bool[size]();
	}

	m_shipSpawned = 0;
	sp_instance = this;
	mp_plan = nullptr;
}

void ai::BlackBoard::IncShip() {
	m_shipSpawned++;
}

float ai::BlackBoard::GetShipSpawnedCount() {
	return m_shipSpawned;
}

void ai::BlackBoard::updateStaticMap(const hlt::Game& kr_game, const int k_adjacentCellRadius)
{
	const int k_size = ai::constants::MAP_SIZE;
	int x, y;	
	ai::constants::AVERAGE_HALITE_PER_CELL = 0.f;
	std::string logger = "";

	//Calcul du Halite Moyen -------------------------------------------------------------------------------------
	for (const std::vector<hlt::MapCell>& vector_line : kr_game.game_map->cells) {
		std::string temp;
		for (const hlt::MapCell& map_cell : vector_line) {
			ai::constants::AVERAGE_HALITE_PER_CELL += map_cell.halite;
		}
	}
	ai::constants::AVERAGE_HALITE_PER_CELL /= (k_size * k_size);


	//On parcourt toutes les cases de la map pour calculer les différentes maps
	for (const std::vector<hlt::MapCell>& vector_line : kr_game.game_map->cells) {
		std::string temp;
		for (const hlt::MapCell& map_cell : vector_line) {
			x = map_cell.position.x;
			y = map_cell.position.y;
		
			//INSPIRATION ----------------------------------------------------------------------------------------
			unsigned short numberOfInspirations = 0;
			if (kr_game.turn_number >= ai::constants::INSPIRATION_STARTING_TURN) {
				for (int localY = hlt::constants::INSPIRATION_RADIUS; localY >= -hlt::constants::INSPIRATION_RADIUS; localY--) {
					for (int localX = abs(localY) - hlt::constants::INSPIRATION_RADIUS; localX <= hlt::constants::INSPIRATION_RADIUS - abs(localY); localX++) {
						hlt::MapCell* currentCell = kr_game.game_map->at(hlt::Position(x + localX, y + localY));
						if (currentCell->is_occupied() && currentCell->ship->owner != kr_game.me->id) {
							numberOfInspirations++;
						}
					}
				}
			}
			//logger += std::to_string(INSPI(numberOfInspirations)) + " ";
			pp_inspirationArray[x][y] = INSPI(numberOfInspirations);
			
			//DISTANCE DROPOFF -----------------------------------------------------------------------------------
			
			// Gestion du shipyard
			hlt::Position closestDropoff = mkr_game.me->shipyard->position;
			hlt::Position position = hlt::Position(x, y);
			int min = mkr_game.game_map->calculate_distance(hlt::Position(x, y), closestDropoff);

			// Gestion des dropoffs
			for (const std::pair<hlt::EntityId, std::shared_ptr<hlt::Dropoff>>& dropoff : mkr_game.me->dropoffs) {
				int distance = mkr_game.game_map->calculate_distance(position, dropoff.second->position);
				if (distance <= min) {
					min = distance;
					closestDropoff = dropoff.second->position;
				}
			}

			//Getion des souhaits de depot de dropoff
			if (mp_plan != nullptr) {
				int planDistance = mkr_game.game_map->calculate_distance(position, mp_plan->position);
				if (planDistance < min) {
					min = planDistance;
					closestDropoff = mp_plan->position;
				}
			}
			
			//logger += std::to_string(min) + " ";
			pp_distanceToDropOffsArray[x][y] = min;

			//HALITE HEAT MAP (PAS UTILISE, compte pour preuve de recherche) ----------------------------------------------------------------------------------
			/*float coefficient = 0.f;
			for (int localY = ai::constants::HALITE_FLATTENING_RADIUS; localY >= -ai::constants::HALITE_FLATTENING_RADIUS; localY--) {
				for (int localX = abs(localY) - ai::constants::HALITE_FLATTENING_RADIUS; localX <= ai::constants::HALITE_FLATTENING_RADIUS - abs(localY); localX++) {
					float tempCoefficient = calculatePseudoNormalLaw(abs(localX) + abs(localY),ai::constants::HALITE_DISTRIBUTION_CURVE_MULTIPLIER, ai::constants::HALITE_DISTRIBUTION_CURVE_SIGMA);
					pp_flattenedHaliteScoreArray[x][y] += ((mkr_game.game_map->at(hlt::Position(x + localX, y + localY))->halite)/ai::constants::AVERAGE_HALITE_PER_CELL) * tempCoefficient;
					coefficient += tempCoefficient;
				}
			}
			pp_flattenedHaliteScoreArray[x][y] /= coefficient;
			logger += std::to_string(pp_flattenedHaliteScoreArray[x][y]) + " ";*/

			//DISPONIBILITE -----------------------------------------------------------------------------------
			//logger += map_cell.is_empty() + " ";
			pp_disponibilityArray[x][y] = map_cell.is_empty();

			//RESET CIBLAGE -----------------------------------------------------------------------------------------
			pp_targetArray[x][y] = false;
		}
		LOG(logger);
	}
	
}

ai::BlackBoard* ai::BlackBoard::sp_instance = nullptr;