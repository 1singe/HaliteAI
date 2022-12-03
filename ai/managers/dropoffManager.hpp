#pragma once

#include "../../hlt/game.hpp"
#include "../../hlt/position.hpp"
#include "../utils/customConstants.hpp"

namespace ai {

	struct DropoffPlan {
		int haliteAt = -1;
		int estimatedCost = INFINITY;
		hlt::Position position;
		hlt::EntityId builder = -1;
	};

	class DropoffManager {
	public:
		DropoffManager(const hlt::Game& kr_game);
		~DropoffManager();

		//detruit le plan en cours
		void deletePlan();
		
		//indique s'il faut construire un dropoff ce tour, et si oui stoque le pkan dans mp_currentPlan
		bool shouldBuildADropoff();

		//retourne le plan de construction en cours
		DropoffPlan* const getDropoffPlan() const ;

	private:
		//retourne le vaisseau le plus aapté à construire un dropoff autour d'une position donné
		std::pair<std::shared_ptr<hlt::Ship>, int> getBuilder(hlt::Position dropoffPosition) const;
		const hlt::Game& mr_game;
		DropoffPlan* mp_currentPlan;

		const float k_maxTurnForBuildingDroppoffRatio = 0.78f;
		const int k_distanceBetweenDropoff = 15;
		const int k_friendShipRadius = 3;
		const int k_dropoffSumRadius = 3;
		static const int k_maxDropoffCount = 3;
		const int k_shipsNeededToBuild[k_maxDropoffCount] = { 12, 27, 65 };
		const int k_turnNeededToBuild[k_maxDropoffCount] = { 120, 190, 240 };
	};
}