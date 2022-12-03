#pragma once

#include "../hlt/player.hpp"
#include "../hlt/game_map.hpp"
#include "utils/customConstants.hpp"
#include "nav/astar.hpp"

#include <map>
#include <vector>

namespace ai {
	class HaliteToAstarLinker {
	public:
		std::map <hlt::EntityId, std::deque<nav::Node>> m_shipsPaths;
		const std::shared_ptr<nav::Astar> m_astar;

		HaliteToAstarLinker(const std::shared_ptr<nav::Astar>& kr_astar, hlt::GameMap& r_gameMap);

		//met a jour Astar en desactivant les transitions dangereuses autour des ennemis pour ce tour
		void updateAstar(const std::vector<std::shared_ptr<hlt::Player>>& kr_players, hlt::PlayerId playerId);

		//desactive une transition ce tour uniquemenr
		void disableTransition(const nav::Transition& kr_transition);

		//retourne si le bateau a déjà un chemin à suivre ou non
		bool hasAPath(hlt::EntityId ship) const;

		//ajoute le plus petit detour possible au chemin pour eviter un ennemi sur la prochaine case
		bool updatePath(const hlt::Position& kr_from, std::deque<ai::nav::Node>& r_path, const int k_overridenCost = -1);

	private:
		hlt::GameMap& mr_gameMap;
		std::set<nav::Transition> m_editedTransitionsOnPreviousTurn;

		std::deque<ai::nav::Node>::const_iterator getNextFreeNode(const std::deque<ai::nav::Node>& kr_path) const;
	};
}