#pragma once

#include "btree/bTreeDecorator.hpp"
#include "btree/bTreeSelector.hpp"
#include "btree/bTreeSequencer.hpp"
#include "btree/bTreeLeaf.hpp"
#include "../hlt/command.hpp"
#include "../hlt/ship.hpp"
#include "blackBoard.hpp"
#include "utils/customConstants.hpp"

#define SUCCESS ai::btree::BtreeNode::State::SUCCESS
#define FAILURE ai::btree::BtreeNode::State::FAILURE
#define RUNNING ai::btree::BtreeNode::State::RUNNING
#define BB ai::BlackBoard::I()

namespace ai {

	struct BTreeData {
		hlt::Ship* const p_ship;
		std::vector<hlt::Command>* const p_commandQueue;
	};

	class HaliteToBTreeLinker {
	public:

		HaliteToBTreeLinker();

		//deroule l'arbre pour un vaisseau donné et rajoute une commande a la command queue
		void evaluate(hlt::Ship* p_ship, std::vector <hlt::Command>* p_commandQueue) const;

	private:
		btree::BtreeNode* p_root;
	};
}
