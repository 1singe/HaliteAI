#include "halite_to_btree.hpp"
#include "../ai/utils/utils.hpp"

//################################################
//##				FUNCTION FOR NODES		    ## 
//################################################

//Met a jour  les transitions interdites/autoris�es suivant le d�placement du bateau
void updateFriendlyShipCollisions(const ai::nav::Node& kr_from, const ai::nav::Node& kr_to) {
	if (kr_from != kr_to) {
		for (const ai::nav::Transition& kr_transition : BB->mr_hltToAstar.m_astar->getTransitionsFromNode(kr_from)) {
			BB->mr_hltToAstar.m_astar->setTransitionEnable({ kr_transition.to, kr_transition.from }, true);
		}
		for (const ai::nav::Transition& kr_transition : BB->mr_hltToAstar.m_astar->getTransitionsFromNode(kr_to)) {
			BB->mr_hltToAstar.disableTransition({ kr_transition.to, kr_transition.from });
		}
		BB->mkr_game.game_map->at({ (int)kr_to.getX() , (int)kr_to.getY() })->mark_unsafe(BB->mkr_game.game_map->at({ (int)kr_from.getX() , (int)kr_from.getY() })->ship);
		BB->mkr_game.game_map->at({ (int)kr_from.getX() , (int)kr_from.getY() })->ship = nullptr;
	}
}

//Indique si le deplacement dans le direction ne causera pas de collision
bool isMoveSafe(hlt::Ship* p_ship, const hlt::Position destination) {
	const ai::nav::Node k_from = { (float)p_ship->position.x, (float)p_ship->position.y };
	const ai::nav::Node k_to = { (float)destination.x, (float)destination.y };

	if (k_from == k_to) {
		return true;
	}
	if (BB->mr_hltToAstar.m_astar->getIsTransitionEnable({ k_from, k_to }) && !BB->mkr_game.game_map->at(destination)->is_occupied()){
		return true;
	}
	return false;
}

//Indique si le bateau a assez de halite pour bouger ce tour
bool hasEnoughHaliteToMove(hlt::Ship* p_ship) {
	return (p_ship->halite >= ( BB->mkr_game.game_map->at(p_ship->position)->halite / 10.0f));
}

//Retourne le dropoff/shipyard le plus proche du bateau
hlt::Position getClosestDropoff(hlt::Ship* p_ship) {
	hlt::Position closestDropoff = BB->mkr_game.me->shipyard->position;
	int minDistance = BB->mkr_game.game_map->calculate_distance(p_ship->position, closestDropoff);

	for (const std::pair<hlt::EntityId, std::shared_ptr<hlt::Dropoff>>& kr_dropoff : BB->mkr_game.me->dropoffs) {
		int newDistance = BB->mkr_game.game_map->calculate_distance(p_ship->position, kr_dropoff.second->position);
		if (newDistance <= minDistance) {
			minDistance = newDistance;
			closestDropoff = kr_dropoff.second->position;
		}
	}

	return closestDropoff;
}

//Reste sur place, envoie la commande et retire les transitions autour
void stay(ai::BTreeData* p_param) {
	hlt::Ship* p_ship = p_param->p_ship;

	for (const ai::nav::Transition& kr_transition : BB->mr_hltToAstar.m_astar->getTransitionsFromNode({ (float)p_ship->position.x, (float)p_ship->position.y })) {
		BB->mr_hltToAstar.disableTransition({ kr_transition.to, kr_transition.from });
	}

	p_param->p_commandQueue->push_back(p_ship->stay_still());
}

//Bouge dans une direction aleatoire sans collision et envoie la commande, retourne false si a du rester sur place
void makeRandomMoveSafe(ai::BTreeData* p_param, const hlt::Direction& kr_wishedDirection, bool canGoBack = true) {
	hlt::Ship* p_ship = p_param->p_ship;

	hlt::Direction directions[4];

	int random = rand() % 2;
	switch (kr_wishedDirection)
	{

	case hlt::Direction::NORTH:
		if (random == 0) {
			directions[0] = hlt::Direction::EAST;
			directions[1] = hlt::Direction::WEST;
		}
		else {
			directions[1] = hlt::Direction::EAST;
			directions[0] = hlt::Direction::WEST;
		}
		directions[2] = hlt::Direction::SOUTH;
		break;

	case hlt::Direction::SOUTH:
		if (random == 0) {
			directions[0] = hlt::Direction::EAST;
			directions[1] = hlt::Direction::WEST;
		}
		else {
			directions[1] = hlt::Direction::EAST;
			directions[0] = hlt::Direction::WEST;
		}
		directions[2] = hlt::Direction::NORTH;
		break;

	case hlt::Direction::EAST:
		if (random == 0) {
			directions[0] = hlt::Direction::NORTH;
			directions[1] = hlt::Direction::SOUTH;
		}
		else {
			directions[1] = hlt::Direction::NORTH;
			directions[0] = hlt::Direction::SOUTH;
		}
		directions[2] = hlt::Direction::WEST;
		break;

	case hlt::Direction::WEST:
		if (random == 0) {
			directions[0] = hlt::Direction::NORTH;
			directions[1] = hlt::Direction::SOUTH;
		}
		else {
			directions[1] = hlt::Direction::NORTH;
			directions[0] = hlt::Direction::SOUTH;
		}
		directions[2] = hlt::Direction::EAST;
		break;
	default:
		directions[0] = hlt::Direction::SOUTH;
		directions[1] = hlt::Direction::NORTH;
		directions[2] = hlt::Direction::EAST;
		directions[3] = hlt::Direction::WEST;
		break;
	}
	directions[3] = kr_wishedDirection;

	//test les directions une a une
	for (int iDir = 0; iDir < 4; iDir++) {
		if (iDir == 2 && !canGoBack) {
			continue;
		}

		hlt::Position randomPosition = BB->mkr_game.game_map->normalize(p_ship->position.directional_offset(directions[iDir]));
		
		if (isMoveSafe(p_ship, randomPosition)) {
			p_param->p_commandQueue->push_back(p_ship->move(directions[iDir]));
			updateFriendlyShipCollisions({ (float)p_ship->position.x, (float)p_ship->position.y }, { (float)randomPosition.x, (float)randomPosition.y });
			return;
		}
	}

	//si aucun deplacement n'est possible, reste sur place
	stay(p_param);
}

//Suit le chemin, envoie la commande et met a jour la position dans Astar, si ne peut pas se rendre ou il veut, recalcule le chemin 
void followThePathSafe(ai::BTreeData* p_param, const int k_overridenCost = -1, bool canGoBack = true) {
	hlt::Ship* p_ship = p_param->p_ship;

	if (!BB->mr_hltToAstar.hasAPath(p_ship->id)) {
		return;
	}
	ai::nav::Node nextPos = BB->mr_hltToAstar.m_shipsPaths[p_ship->id].front();

	//si la prochiane case ou se rendre n'est pas safe, alors recalcule un chemin entre la position et la prochaine case safe sur le chemin
	bool canMove = isMoveSafe(p_ship, { (int)nextPos.getX(), (int)nextPos.getY() });
	if (!canMove) {
		canMove = BB->mr_hltToAstar.updatePath(p_ship->position, BB->mr_hltToAstar.m_shipsPaths[p_ship->id], k_overridenCost);
		if (canMove) {
			nextPos = BB->mr_hltToAstar.m_shipsPaths[p_ship->id].front();
		}
	}

	//regarde dans quelle direction bouger pour avancer sur le chemin et avance en mettant a jour les collisions
	hlt::Direction wishedDirection = hlt::Direction::NORTH;
	if (canMove) {
		for (hlt::Direction direction : hlt::ALL_CARDINALS) {
			const hlt::Position k_nextShipPosition = BB->mkr_game.game_map->normalize(p_ship->position.directional_offset(direction));
			const ai::nav::Node k_node((float)k_nextShipPosition.x, (float)k_nextShipPosition.y);
			if (k_node == nextPos) {
				if (isMoveSafe(p_ship, k_nextShipPosition)) {
					p_param->p_commandQueue->push_back(p_ship->move(direction));
					updateFriendlyShipCollisions({ (float)p_ship->position.x, (float)p_ship->position.y }, nextPos);
					BB->mr_hltToAstar.m_shipsPaths[p_ship->id].pop_front();
					wishedDirection = direction;
				}
				else {
					wishedDirection = direction;
					break;
				}
				return;
			}
		}
	}
	//si il y a une collision sur le chemin, fait un mouvement aleatoire, utile pour decoincer un noeud
	makeRandomMoveSafe(p_param, wishedDirection, canGoBack);
}

//Avance sur le chemin peut importe les collisions
void followThePathUnsafe(ai::BTreeData* p_param) {
	hlt::Ship* p_ship = p_param->p_ship;

	if (!BB->mr_hltToAstar.hasAPath(p_ship->id)) {
		return;
	}
	const ai::nav::Node k_nextPos = BB->mr_hltToAstar.m_shipsPaths[p_ship->id].front();

	//regarde dans quelle direction bouger pour avancer sur le chemin et avance en mettant a jour les collisions
	for (hlt::Direction direction : hlt::ALL_CARDINALS) {
		const hlt::Position k_nextShipPosition = BB->mkr_game.game_map->normalize(p_ship->position.directional_offset(direction));
		const ai::nav::Node k_node((float)k_nextShipPosition.x, (float)k_nextShipPosition.y);
		if (k_node == k_nextPos) {
			p_param->p_commandQueue->push_back(p_ship->move(direction));
			updateFriendlyShipCollisions({ (float)p_ship->position.x, (float)p_ship->position.y }, k_node);
			BB->mr_hltToAstar.m_shipsPaths[p_ship->id].pop_front();
			return;
		}
	}	

	//ne dervait pas arriver
	stay(p_param);
}


//Retourne la position de l'ennemie le plus proche, si il n'y a pas d'ennemie retourne la position du bateau qui appelle la fonction
hlt::Position findClosestEnemy( const hlt::Position& source) {
	hlt::Position closestShip = source;
	int closestDistance = ai::constants::MAP_SIZE * ai::constants::MAP_SIZE;

	for (std::shared_ptr<hlt::Player> player : BB->mkr_game.players) {
		if (player->id != BB->mkr_game.me->id) {
			for (const std::pair<hlt::EntityId, std::shared_ptr<hlt::Ship>>& ship : player->ships) {
				int distance = BB->mkr_game.game_map->calculate_distance(source, ship.second->position);
				if (distance < closestDistance && !BB->mkr_game.game_map->at(ship.second->position)->has_structure()) {
					closestDistance = distance;
					closestShip = ship.second->position;
				}
			}
		}
	}

	return closestShip;
}

bool shouldMoveAlongPath(const hlt::Ship* p_ship, const hlt::Position& destination) {
	float bonusHaliteOnCurrentCell = BB->mkr_game.game_map->at(p_ship->position)->halite * float(1.0f / float(hlt::constants::EXTRACT_RATIO));
	bonusHaliteOnCurrentCell = p_ship->halite + bonusHaliteOnCurrentCell > hlt::constants::MAX_HALITE ? hlt::constants::MAX_HALITE - p_ship->halite : bonusHaliteOnCurrentCell;
	float bonusHaliteOnNextCell = BB->mkr_game.game_map->at(destination)->halite * float(1.0f / float(hlt::constants::EXTRACT_RATIO));
	int moveCost = BB->mkr_game.game_map->at(p_ship)->halite * float(1.0f / hlt::constants::MOVE_COST_RATIO);
	// Calcule en prenant compte que cela fait perdre des halite et 1 tour d'extraction
	float moveScore = bonusHaliteOnNextCell * float(1.0f / float(hlt::constants::EXTRACT_RATIO)) - moveCost + (bonusHaliteOnCurrentCell * ai::constants::EXPANSIONISM);
	float stayScore = bonusHaliteOnCurrentCell;

	if (stayScore > moveScore) {
		return false;
	}
	else {
		return true;
	}
}

//regarde dans les quatre directions cardinales et a la position quel mouvement rapporterai le plus de halite
hlt::Direction getMostHaliteAround(const hlt::Position& k_source) {
	float maxHalite = BB->mkr_game.game_map->at(k_source)->halite*float(1.0f/float(hlt::constants::EXTRACT_RATIO));
	int moveCost = BB->mkr_game.game_map->at(k_source)->halite *float(1.0f/hlt::constants::MOVE_COST_RATIO);
	hlt::Direction bestDirection = hlt::Direction::STILL;

	for (hlt::Direction direction : hlt::ALL_CARDINALS) {
		float halite = BB->mkr_game.game_map->at(BB->mkr_game.game_map->normalize(k_source.directional_offset(direction)))->halite * float(1.0f / float(hlt::constants::EXTRACT_RATIO)) - moveCost;
		if (halite >= maxHalite) {
			maxHalite = halite;
			bestDirection = direction;
		}
	}
	return bestDirection;
}

//regarde sur TOUTE la map la meilleure destination possible a l'aide de differents scores
hlt::Position getBestScoredTile(const hlt::Position& kr_source, float** pp_scoreArray) {
	float max = -std::numeric_limits<float>::max();
	hlt::Position position = kr_source;
	for (size_t y = 0; y < ai::constants::MAP_SIZE; y++) {
		for (size_t x = 0; x < ai::constants::MAP_SIZE; x++) {
			float score = pp_scoreArray[x][y];
			if (score > max) {
				max = score;
				position = hlt::Position(x, y);
			}
		}
	}
	return position;
}

//dirige un ship vers le dropoff le plus proche
void addPathToClosestDropoff(hlt::Ship* p_ship) {
	const hlt::Position k_toPos = getClosestDropoff(p_ship);
	const ai::nav::Node k_to = { (float)k_toPos.x, (float)k_toPos.y };
	const ai::nav::Node k_from = { (float)p_ship->position.x, (float)p_ship->position.y };
	const ai::nav::PathData k_path = BB->mr_hltToAstar.m_astar->getPath(k_from, k_to, -1, true, ai::constants::MAX_ITERATIONS_WITHOUT_GETTING_CLOSER_ALLOWED, ai::constants::AVERAGE_HALITE_PER_CELL);
	BB->mr_hltToAstar.m_shipsPaths[p_ship->id] = k_path.path;
}

//retourne une map contenant le score pour chaque direction que le vaisseau peut prendre
std::map<hlt::Direction, float> getDirectionsScores(const hlt::Position& kr_source, const hlt::Position& kr_destination) {
	using dir = hlt::Direction;
	std::map<hlt::Direction, float> directionsScores = std::map<hlt::Direction, float>();
	int dx = kr_destination.x - kr_source.x;
	int dy = kr_destination.y - kr_source.y;

	dx = abs(dx) < A_MOD(dx, ai::constants::MAP_SIZE) ? dx : A_MOD(dx, ai::constants::MAP_SIZE);
	dy = abs(dy) < A_MOD(dy, ai::constants::MAP_SIZE) ? dy : A_MOD(dy, ai::constants::MAP_SIZE);
	LOG("Vector : " + std::to_string(dx) + ", " + std::to_string(dy));


	int signOfDx = getSign(dx);
	int signOfDy = getSign(dy);
	std::string temp = "";
	for (const dir direction : hlt::ALL_CARDINALS) {
		float score = 0;
		float nextCellHaliteRatio = BB->mkr_game.game_map->at(BB->mkr_game.game_map->normalize(kr_source.directional_offset(direction)))->halite / ai::constants::MAX_FILLED_CELL_AMOUNT;
		switch (direction) {
		case dir::NORTH:
			score = -signOfDy + nextCellHaliteRatio;
			temp += "NORTH : ";
			break;
		case dir::SOUTH:
			score = signOfDy + nextCellHaliteRatio;
			temp += "SOUTH : ";
			break;
		case dir::WEST:
			score = -signOfDx + nextCellHaliteRatio;
			temp += "WEST : ";
			break;
		case dir::EAST:
			score = signOfDx + nextCellHaliteRatio;
			temp += "EAST : ";
			break;
		default:
			break;
		}
		temp += std::to_string(score) + " | ";
		directionsScores.try_emplace(direction, score);
	}
	LOG(temp);
	return directionsScores;
}

//retourne la direction que le vaisseau doit prendre
hlt::Direction getBestScoredDirection(std::map<hlt::Direction, float> directionsScores) {
	if (directionsScores.empty()) return hlt::Direction::STILL;
	float max = -std::numeric_limits<float>::max();
	hlt::Direction direction = hlt::Direction::STILL;
	for (const std::pair<hlt::Direction, float> entry : directionsScores) {
		if (entry.second > max) {
			direction = entry.first;
			max = entry.second;
		}
	}
	return direction;
}


//################################################
//##				NODES BEHAVIOUR			    ## 
//################################################
using namespace ai::btree;

//####### DROPOFF #######

//retourne SUCCESS si doit construire un dropoff, sinon retourne failure
BtreeNode::State hasToBuildADropoff(void* param) {
	ai::BTreeData* p_param = (ai::BTreeData*)param;
	hlt::Ship* p_ship = p_param->p_ship;
	LOG("HasToBuildADropoff : " + std::to_string(p_ship->id));

	if (BB->mp_plan != nullptr) {
		if (BB->mp_plan->builder == p_ship->id) {
			return SUCCESS;
		}
	}
	return FAILURE;
}

//retourne SUCCESS si reussi a construire un dropoff, sinon FAILURE;
BtreeNode::State buildDropoff(void* param) {
	ai::BTreeData* p_param = (ai::BTreeData*)param;
	hlt::Ship* p_ship = p_param->p_ship;
	LOG("BuildDropoff : " + std::to_string(p_ship->id));

	hlt::Position buildPosition = BB->mp_plan->position;
	if (p_ship->position == buildPosition) {
		int buildingCost = hlt::constants::DROPOFF_COST - BB->mkr_game.game_map->at(buildPosition)->halite - p_ship->halite;
		if (buildingCost < BB->mkr_game.me->halite) {
			p_param->p_commandQueue->push_back(p_ship->make_dropoff());
			BB->mr_hltToAstar.m_shipsPaths[p_ship->id].clear();
			BB->m_hasBuiltADropoff = true;
			BB->mp_plan->builder = -1;
			BB->mp_plan = nullptr; 
			return SUCCESS;
		}
	}
	return FAILURE;
}

//avance vers le prochain lieu de construction de dropoff, retourne toujours SUCCESS
BtreeNode::State goToFuturDropoff(void* param) {
	ai::BTreeData* p_param = (ai::BTreeData*)param;
	hlt::Ship* p_ship = p_param->p_ship;
	LOG("GoToFuturDropoff : " + std::to_string(p_ship->id));

	if ( hasEnoughHaliteToMove(p_ship) && p_ship->position != BB->mp_plan->position) {
		//si il n'a pas de chemin deja calcule ,calcule un chemin cours en consommant le moins d'halite possible
		if (!BB->mr_hltToAstar.hasAPath(p_ship->id)) {
			ai::nav::Node from = { (float)p_ship->position.x, (float)p_ship->position.y };
			ai::nav::Node to = { (float)BB->mp_plan->position.x,(float)BB->mp_plan->position.y };
			BB->mr_hltToAstar.m_shipsPaths[p_ship->id] = BB->mr_hltToAstar.m_astar->getPath(from, to, -1, false, ai::constants::MAX_ITERATIONS_WITHOUT_GETTING_CLOSER_ALLOWED, ai::constants::AVERAGE_HALITE_PER_CELL).path;
		}
		else {
			ai::nav::Node to = { (float)BB->mp_plan->position.x,(float)BB->mp_plan->position.y };
			//si le lieu de construction de dropoff a change, recalcule un chemin
			if (BB->mr_hltToAstar.m_shipsPaths[p_ship->id].back() != to) {
				ai::nav::Node from = { (float)p_ship->position.x, (float)p_ship->position.y };
				BB->mr_hltToAstar.m_shipsPaths[p_ship->id] = BB->mr_hltToAstar.m_astar->getPath(from, to, -1, false, ai::constants::MAX_ITERATIONS_WITHOUT_GETTING_CLOSER_ALLOWED, ai::constants::AVERAGE_HALITE_PER_CELL).path;
			}
		}
		followThePathSafe(p_param);
	}
	//si le bateau n'a pas assez de halite pour bouger ou construire, rester sur place
	else {
		stay(p_param);
	}
	return SUCCESS;
}


//####### RETURNING TO DROPOFF #######

//va deposer du halite, retourne failure si n a pas assez de HALITE pour en deposer, sinon retourne SUCCESS
BtreeNode::State depositHalite(void* param) {
	ai::BTreeData* p_param = (ai::BTreeData*)param;
	hlt::Ship* p_ship = p_param->p_ship;
	LOG("DepositHalite : " + std::to_string(p_ship->id));

	//si il n'est pas en train de deposer
	if (BB->m_deposingShips.find(p_ship->id) == BB->m_deposingShips.end()) {
		if (p_ship->halite >= hlt::constants::MAX_HALITE * float((ai::constants::COLLECT_PERCENTAGE_BEFORE_BACK/100.0f))) {
			//donner un chemin si il a assez de halite pour le deposer
			addPathToClosestDropoff(p_ship);
			BB->m_deposingShips.emplace(p_ship->id);
		}
		//sinon, ira en recolter
		else {
			return FAILURE;
		}
	}


	if (BB->m_deposingShips.find(p_ship->id) != BB->m_deposingShips.end() && p_ship->halite >= hlt::constants::MAX_HALITE * float((ai::constants::COLLECT_PERCENTAGE_CANCEL_BACK / 100.0f))) {
		//suit le chemin
		if (hasEnoughHaliteToMove(p_ship)) {
			std::shared_ptr<hlt::Entity> p_structure = BB->mkr_game.game_map->at(p_ship->position)->structure;
			//si le ship vient de deposer, supprimer le chemin
			if (p_structure.get() != nullptr) {
				if (p_structure->owner == BB->mkr_game.me->id) {
					BB->mr_hltToAstar.m_shipsPaths[p_ship->id].clear();
					return FAILURE;
				}
			}

			addPathToClosestDropoff(p_ship);

			if (BB->mr_hltToAstar.m_shipsPaths[p_ship->id].size() <= ai::constants::UNSAFE_RADIUS_AROUND_DROPOFF) {
				ai::nav::Node destinationNode = BB->mr_hltToAstar.m_shipsPaths[p_ship->id].back();
				hlt::Position destination = { (int)destinationNode.getX(), (int)destinationNode.getY() };
				ai::nav::Node nextNode = BB->mr_hltToAstar.m_shipsPaths[p_ship->id].front();
				hlt::Position next = { (int)nextNode.getX(), (int)nextNode.getY() };

				if (BB->mkr_game.game_map->at(destination)->has_structure()) {
					hlt::PlayerId playerId = BB->mkr_game.me->id;
					//si dropoff occupe allie et par un vaisseau ennemi, ou libre, foncer
					if ((BB->mkr_game.game_map->at(next)->is_occupied() && BB->mkr_game.game_map->at(next)->ship->owner != playerId)) {
						followThePathUnsafe(p_param);
						return SUCCESS;
					}
				}
			}
			followThePathSafe(p_param);
		}
		else {
			stay(p_param);
		}
		return SUCCESS;
	}
	//efface le chemin de retour car plus assez de halite
	else {
		BB->mr_hltToAstar.m_shipsPaths[p_ship->id].clear();
		BB->m_deposingShips.erase(p_ship->id);
		return FAILURE;
	}
}

//va deposer le halite s'il ne reste plus beaucoup de tours
BtreeNode::State urgentDepositHalite(void* param) {
	ai::BTreeData* p_param = (ai::BTreeData*)param;
	hlt::Ship* p_ship = p_param->p_ship;
	LOG("UrgentDepositHalite : " + std::to_string(p_ship->id));

	int scaledBonusTurn = std::max((float)ai::constants::TURN_BONUS_TO_SOLVE_CONFLITS, ai::constants::TURN_BONUS_TO_SOLVE_CONFLITS * (BB->GetShipSpawnedCount() / ai::constants::MINIMUM_SHIP_FOR_BONUS));
	const int k_turnLeft = hlt::constants::MAX_TURNS - BB->mkr_game.turn_number;
	if (k_turnLeft > ai::constants::MAP_SIZE + scaledBonusTurn) {
		return FAILURE;
	}
	else {
		const hlt::Position k_closestDropoff = getClosestDropoff(p_ship);
		const int k_distance = BB->mkr_game.game_map->calculate_distance(p_ship->position, k_closestDropoff);
		
		if (k_distance >= k_turnLeft - scaledBonusTurn) {

			if (k_distance == 0 || !hasEnoughHaliteToMove(p_ship)) {
				stay(p_param);
				return SUCCESS;
			}

			//donner un chemin avec cout a 1 pour rentrer vite
			const hlt::Position k_toPos = getClosestDropoff(p_ship);
			const ai::nav::Node k_to = { (float)k_toPos.x, (float)k_toPos.y };
			const ai::nav::Node k_from = { (float)p_ship->position.x, (float)p_ship->position.y };
			BB->mr_hltToAstar.m_shipsPaths[p_ship->id] = BB->mr_hltToAstar.m_astar->getPath(k_from, k_to, 1, true, ai::constants::MAX_ITERATIONS_WITHOUT_GETTING_CLOSER_ALLOWED).path;
			if (BB->mr_hltToAstar.m_shipsPaths[p_ship->id].size() == 1) {
				followThePathUnsafe(p_param);
			}
			else if (k_distance <= k_turnLeft + 1 ) {
				ai::nav::Node nextPosNode = BB->mr_hltToAstar.m_shipsPaths[p_ship->id].front();
				hlt::Position nextPos = { (int)nextPosNode.getX(), (int)nextPosNode.getY() };
				std::shared_ptr<hlt::Ship> p_blockingShip = BB->mkr_game.game_map->at(nextPos)->ship;

				if (p_blockingShip != nullptr) {
					if (p_blockingShip->owner != BB->mkr_game.me->id) {
						followThePathUnsafe(p_param);
					}
					else if (p_ship->position.x == k_toPos.x || p_ship->position.y == k_toPos.y) {
						stay(p_param);
					}
					else
					{
						followThePathSafe(p_param, 1, false);
					}
				}
				else {
					followThePathUnsafe(p_param);
				}
			}
			else {
				//trop loin pour rentrer, mais peut se couler sur un ennemi 
				const hlt::Position k_closestEnnemy = findClosestEnemy(p_ship->position);
				const ai::nav::Node k_closestEnnemyNode = { (float)k_closestEnnemy.x, (float)k_closestEnnemy.y };
				BB->mr_hltToAstar.m_shipsPaths[p_ship->id] = BB->mr_hltToAstar.m_astar->getPath(k_from, k_closestEnnemyNode, 1, true, ai::constants::MAX_ITERATIONS_WITHOUT_GETTING_CLOSER_ALLOWED).path;
				const ai::nav::Node k_nextPosNode = BB->mr_hltToAstar.m_shipsPaths[p_ship->id].front();
				const hlt::Position k_nextPos = { (int)k_nextPosNode.getX(), (int)k_nextPosNode.getY() };
				std::shared_ptr<hlt::Ship> p_blockingShip = BB->mkr_game.game_map->at(k_nextPos)->ship;
				if (p_blockingShip != nullptr) {
					if (p_blockingShip->owner != BB->mkr_game.me->id) {
						followThePathUnsafe(p_param);
					}
					else {
						followThePathSafe(p_param, 1, true);
					}
				} else {
					followThePathUnsafe(p_param);
				}
			}
			return SUCCESS;
		}
		return FAILURE;
	}
}

//####### COLLECTING DROPOFF #######

//fonction de collecte naive, faible temps GPU donc utile comme adversaire pour tester
BtreeNode::State getMostHaliteNaif(void* param) {
	ai::BTreeData* p_param = (ai::BTreeData*)param;
	hlt::Ship* p_ship = p_param->p_ship;
	LOG("getMostHaliteNaif : " + std::to_string(p_ship->id));

	if (hasEnoughHaliteToMove(p_ship)) {
		hlt::Direction desiredDirection = getMostHaliteAround(p_ship->position);
		hlt::Position desiredPosition = BB->mkr_game.game_map->normalize(p_ship->position.directional_offset(desiredDirection));
		if (isMoveSafe(p_ship, desiredPosition)) {
			p_param->p_commandQueue->push_back(p_ship->move(desiredDirection));
			updateFriendlyShipCollisions({ (float)p_ship->position.x, (float)p_ship->position.y }, { (float)desiredPosition.x, (float)desiredPosition.y });
			return SUCCESS;
		}
		else {
			makeRandomMoveSafe(p_param, desiredDirection);
			return SUCCESS;
		}
	}
	else {
		stay(p_param);
		return SUCCESS;
	}
}

//fonction de collecte avancee
BtreeNode::State moveTowardsBestTile(void* param) {
	ai::BTreeData* p_param = (ai::BTreeData*)param;
	hlt::Ship* p_ship = p_param->p_ship;
	const int k_size = ai::constants::MAP_SIZE;
	LOG("getMostHaliteAdvanced : " + std::to_string(p_ship->id));

	//Creer le tabelau de score propre au ship en copiant le tableau de score du blackboard
	float** pp_ownScoreArray = new float* [k_size];
	for (int x = 0; x < k_size; x++) {
		pp_ownScoreArray[x] = new float[k_size]();
	}

	float distanceFromShipMalus;
	int distanceFromShip;
	float distanceFromDropOffMalus;
	int distanceFromDropOff;
	float haliteScore;
	float inspirationMultiplier;
	bool available;
	float targettedMalus;

	for (const std::vector<hlt::MapCell>& kr_vectorLine : BB->mkr_game.game_map->cells) {
		std::string temp;
		for (const hlt::MapCell& kr_mapCell : kr_vectorLine) {
			int x = kr_mapCell.position.x;
			int y = kr_mapCell.position.y;

			//---DISTANCE AU SHIP---
			//Pondere le score personnel en fonction de la distance et de l'halite moyen a cette meme distance
			//Grand risque de time out pour de longues distances > switch a une estimation naive a plus de MAX_DISTANCE_FOR_ADVANCED_AVERAGE
			float averageHaliteBetweenShip = ai::constants::AVERAGE_HALITE_PER_CELL;

			distanceFromShip = BB->mkr_game.game_map->calculate_distance(p_ship->position, kr_mapCell.position);

			//Calcul avance, seulement si la distance est inferieur a MAX_DISTANCE_FOR_ADVANCED_VERAGE
			int numberOfTiles = 0;
			if (distanceFromShip <= ai::constants::MAX_DISTANCE_FOR_ADVANCED_AVERAGE) {
				for (int localY = distanceFromShip; localY >= -distanceFromShip; localY--) {
					for (int localX = abs(localY) - distanceFromShip; localX <= distanceFromShip - abs(localY); localX++) {
						numberOfTiles++;
						averageHaliteBetweenShip += BB->mkr_game.game_map->at(hlt::Position(localX, localY))->halite;
					}
				}
				averageHaliteBetweenShip /= numberOfTiles;
			}
			distanceFromShipMalus = distanceFromShip * averageHaliteBetweenShip * (1.0f / hlt::constants::MOVE_COST_RATIO);

			//DISTANCE AU DROPOFF --------------------------------------------------------------------------------------------------
			distanceFromDropOff = BB->pp_distanceToDropOffsArray[x][y];
			distanceFromDropOffMalus = distanceFromDropOff * DROPOFF_DIST_MULT(BB->mkr_game.turn_number, ai::constants::END_DISTANCE_TO_DROPOFF_MULTIPLIER) * ai::constants::AVERAGE_HALITE_PER_CELL * (1.0f / hlt::constants::MOVE_COST_RATIO);

			//INSPIRATION ----------------------------------------------------------------------------------------------------------
			inspirationMultiplier = BB->pp_inspirationArray[x][y];

			//CIBLAGE DE CELLULE ---------------------------------------------------------------------------------------------------
			targettedMalus = BB->pp_targetArray[x][y] * hlt::constants::MAX_HALITE;

			//SCORE ----------------------------------------------------------------------------------------------------------------
			haliteScore = BB->mkr_game.game_map->at(hlt::Position(x, y))->halite;

			pp_ownScoreArray[x][y] = (haliteScore * inspirationMultiplier) - (distanceFromShipMalus * ai::constants::SHIP_DISTANCE_WEIGHT)
				- (distanceFromDropOffMalus * ai::constants::DROPOFF_DISTANCE_WEIGHT) - (targettedMalus * ai::constants::TARGET_WEIGHT);
				 

		}
	}

	//ETALAGE DES VALEURS -----------------------------------------------------------------------------------------------------------
	float** pp_ownScoreArrayCopy = new float* [k_size];
	for (int x = 0; x < k_size; x++) {
		pp_ownScoreArrayCopy[x] = new float[k_size]();
	}

	for (int x = 0; x < k_size; x++) {
		for (int y = 0; y < k_size; y++) {
			pp_ownScoreArrayCopy[x][y] = pp_ownScoreArray[x][y];
		}
	}

	for (const std::vector<hlt::MapCell>& kr_vectorLine : BB->mkr_game.game_map->cells) {
		std::string temp;
		for (const hlt::MapCell& kr_mapCell : kr_vectorLine) {
			int x = kr_mapCell.position.x;
			int y = kr_mapCell.position.y;
			if (!BB->pp_disponibilityArray[x][y]) {
				pp_ownScoreArray[x][y] = -std::numeric_limits<float>::max();
				continue;
			}
			for (int localY = ai::constants::SCORE_FLATTENING_RADIUS; localY >= -ai::constants::AVERAGE_HALITE_PER_CELL; localY--) {
				for (int localX = abs(localY) - ai::constants::SCORE_FLATTENING_RADIUS; localX <= ai::constants::SCORE_FLATTENING_RADIUS - abs(localY); localX++) {
					pp_ownScoreArray[x][y] += pp_ownScoreArrayCopy[A_MOD(x + localX, k_size)][A_MOD(y + localY, k_size)] * HALIT(abs(localX) + abs(localY));
				}
			}
			
		}
	}

	// Custom Algo
	
	hlt::Position desiredPosition = BB->mkr_game.game_map->normalize(getBestScoredTile(p_ship->position, pp_ownScoreArray));

	if (BB->pp_targetArray[desiredPosition.x][desiredPosition.y]) LOG("This tile is already being targetted");
	LOG("moveTowards :id = " + std::to_string(p_ship->id) + " (" + std::to_string(desiredPosition.x) + ", " + std::to_string(desiredPosition.y) + ") for a score of " + std::to_string(pp_ownScoreArray[desiredPosition.x][desiredPosition.y]));
	BB->pp_targetArray[desiredPosition.x][desiredPosition.y] = true;
	std::map<hlt::Direction, float> directionsScores = getDirectionsScores(p_ship->position, desiredPosition);

	for (int x = 0; x < k_size; x++) {
		delete[] pp_ownScoreArray[x];
		delete[] pp_ownScoreArrayCopy[x];
	}
	delete[] pp_ownScoreArray;
	delete[] pp_ownScoreArrayCopy;

	if (hasEnoughHaliteToMove(p_ship)) {
		int i = 0;
		std::string s = "finally went ";
		while (!directionsScores.empty()) {
			hlt::Direction nextDirection = getBestScoredDirection(directionsScores);
			hlt::Position nextPosition = BB->mkr_game.game_map->normalize(p_ship->position.directional_offset(nextDirection));
			if (isMoveSafe(p_ship, nextPosition)) {
				if (shouldMoveAlongPath(p_ship, nextPosition)) {
					s.push_back(static_cast<char>(nextDirection));
					LOG(s);
					p_param->p_commandQueue->push_back(p_ship->move(nextDirection));
					updateFriendlyShipCollisions({ (float)p_ship->position.x, (float)p_ship->position.y }, { (float)nextPosition.x, (float)nextPosition.y });
					return SUCCESS;
				}
				else {
					LOG("finally stays");
					stay(p_param);
					return SUCCESS;
				}
			}
			else {
				directionsScores.erase(nextDirection);
			}
		}
	}
	LOG("All directions have been tested, stays.");
	stay(p_param);
	return SUCCESS;
}
	
//################################################
//##				BEHAVIOUR TREE			    ## 
//################################################

//evalue la racine de l'arbre
void ai::HaliteToBTreeLinker::evaluate(hlt::Ship* p_ship, std::vector <hlt::Command>* p_commandQueue) const {
	BTreeData* p_param = new BTreeData{p_ship, p_commandQueue };
	const int k_queueSize = p_commandQueue->size();

	p_root->evaluate(p_param);

	//si aucune commande a ete ajoutee, rester sur place, ne devrait pas arriver
	if (k_queueSize == p_commandQueue->size()) {
		stay(p_param);
	}

	//si trop de commande ont ete ajoutees, en retirer, ne devrait pas arriver
	while (p_commandQueue->size() > k_queueSize + 1) {
		p_commandQueue->pop_back();
	}
}

//initialisation de l'arbre
ai::HaliteToBTreeLinker::HaliteToBTreeLinker() {
	
	p_root = new BtreeSelector(3); // dropoff > rentrer > collecter du halite

	//relatif aux dropoffs
	BtreeSequencer* p_dropoffBuildingManagement = new BtreeSequencer(2);
	p_dropoffBuildingManagement->addChild(new BtreeLeaf(p_dropoffBuildingManagement, hasToBuildADropoff));
		BtreeSelector* hasToBuild = new BtreeSelector(2);
		hasToBuild->addChild(new BtreeLeaf(hasToBuild,buildDropoff));
		hasToBuild->addChild(new BtreeLeaf(hasToBuild,goToFuturDropoff));
	p_dropoffBuildingManagement->addChild(hasToBuild);

	//relatif au depot de halite
	BtreeSelector* p_depositManagement = new BtreeSelector(2);
	p_depositManagement->addChild(new BtreeLeaf(p_depositManagement, urgentDepositHalite));
	p_depositManagement->addChild(new BtreeLeaf(p_depositManagement, depositHalite));

	//attacher a la racine
	p_root->addChild(p_dropoffBuildingManagement);
	p_root->addChild(p_depositManagement);

	//attacher a la racine le node de collecte de halie
	p_root->addChild(new BtreeLeaf(p_root, moveTowardsBestTile));
}