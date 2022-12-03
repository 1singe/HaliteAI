#pragma once

#include "../hlt/game.hpp"
#include "managers/dropoffManager.hpp"
#include "../hlt/position.hpp"
#include "halite_to_astar.hpp"
#include "../ai/utils/utils.hpp"

namespace ai {

	class BlackBoard {
	public:
		const hlt::Game& mkr_game;
		DropoffPlan* mp_plan;
		HaliteToAstarLinker& mr_hltToAstar;
		bool m_hasBuiltADropoff = false;
		bool** pp_targetArray;
		std::set<hlt::EntityId> m_deposingShips;

		//maps
		float** pp_flattenedHaliteScoreArray;
		float** pp_distanceToDropOffsArray;
		float** pp_inspirationArray;
		bool** pp_disponibilityArray;

		void IncShip();
		float GetShipSpawnedCount();

		static BlackBoard* I(); //singleton
		BlackBoard(const hlt::Game& kr_game, HaliteToAstarLinker& r_hltToAstar);
		void updateStaticMap(const hlt::Game& kr_game, const int k_adjacentCellRadius);

	private:
		static BlackBoard* sp_instance;
		int m_shipSpawned;
	};

}