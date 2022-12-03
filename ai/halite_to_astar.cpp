#include "halite_to_astar.hpp"

ai::HaliteToAstarLinker::HaliteToAstarLinker(const std::shared_ptr<nav::Astar>& kr_astar, hlt::GameMap& r_gameMap)
	: m_astar(kr_astar), mr_gameMap(r_gameMap)
{}

void ai::HaliteToAstarLinker::updateAstar(const std::vector<std::shared_ptr<hlt::Player>>& kr_players, hlt::PlayerId playerId) {
	for (const nav::Transition& kr_transition : m_editedTransitionsOnPreviousTurn) {
		m_astar->setTransitionEnableAndCost(kr_transition, true,
			mr_gameMap.at({ (int)kr_transition.from.getX(), (int)kr_transition.from.getY() })->halite / 10.f);
	}

	m_editedTransitionsOnPreviousTurn.clear();

	for (const std::shared_ptr<hlt::Player>& kr_player : kr_players) {
		if (kr_player->id != playerId) {
			for (const std::pair<hlt::EntityId, std::shared_ptr<hlt::Ship>>& kr_ship : kr_player->ships) {
				//for every ship of the enemy, disable every transition possible arount it to prevent from colliding
				for (const nav::Transition& kr_transition_distance1 : m_astar->getTransitionsFromNode({ (float)kr_ship.second->position.x, (float)kr_ship.second->position.y })) {
					for (const nav::Transition& kr_transition_distance2 : m_astar->getTransitionsFromNode(kr_transition_distance1.to)) {
						disableTransition({ kr_transition_distance2.to, kr_transition_distance2.from });
					}
					disableTransition({ kr_transition_distance1.to, kr_transition_distance1.from });
				}
			}
		}
		else {
			for (const std::pair<hlt::EntityId, std::shared_ptr<hlt::Ship>>& kr_ship : kr_player->ships) {
				for (const nav::Transition& kr_transition : m_astar->getTransitionsFromNode({ (float)kr_ship.second->position.x, (float)kr_ship.second->position.y })) {
					disableTransition({ kr_transition.to, kr_transition.from });
				}
			}
		}
	}
}

void ai::HaliteToAstarLinker::disableTransition(const nav::Transition& kr_transition) {
	m_astar->setTransitionEnable(kr_transition, false);
	m_editedTransitionsOnPreviousTurn.insert(kr_transition);
}

bool ai::HaliteToAstarLinker::hasAPath(hlt::EntityId ship) const {
	if (m_shipsPaths.find(ship) == m_shipsPaths.end()) return false;
	if (m_shipsPaths.find(ship)->second.empty()) return false;
	return true;
}

bool ai::HaliteToAstarLinker::updatePath(const hlt::Position& kr_from, std::deque<ai::nav::Node>& r_path, const int k_overridenCost)
{
	std::deque<ai::nav::Node> startpath = r_path;
	std::deque<ai::nav::Node>::const_iterator it_destination = getNextFreeNode(r_path);
	if (it_destination == r_path.cend()) {
		return false;
	}
	else {
		ai::nav::PathData pathData = m_astar->getPath({ (float)kr_from.x, (float)kr_from.y}, *it_destination, k_overridenCost, false, ai::constants::MAX_ITERATIONS_WITHOUT_GETTING_CLOSER_ALLOWED, constants::AVERAGE_HALITE_PER_CELL);
		if (pathData.goesToPlannedDestination == false) {
			return false;
		}
		else {
			r_path.erase(r_path.begin(), it_destination);
			pathData.path.pop_back();
			std::deque<ai::nav::Node>::iterator endNode;
			bool hasFinished = false;
			for (endNode = pathData.path.begin(); endNode != pathData.path.end(); endNode++) {
				for (int i = 0; i < r_path.size(); i++) {
					if ((*endNode) == r_path[i]) {
						break;
					}
				}
				if (hasFinished) {
					break;
				}
			}
			r_path.insert(r_path.begin(), pathData.path.begin(), endNode);
		}
	}
}

std::deque<ai::nav::Node>::const_iterator ai::HaliteToAstarLinker::getNextFreeNode(const std::deque<ai::nav::Node>& kr_path) const
{
	std::deque<ai::nav::Node>::const_iterator it_nd;
	for ( it_nd = kr_path.cbegin(); it_nd != kr_path.cend(); it_nd++) {
		hlt::Position pos = { (int)it_nd->getX(), (int)it_nd->getY() };
		if (!mr_gameMap.at(pos)->is_occupied()) {
			return it_nd;
		}
	}
	return  kr_path.cend();
}
