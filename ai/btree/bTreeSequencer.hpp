#pragma once

#include "bTreeNode.hpp" 

namespace ai {
	namespace btree {

        class BtreeSequencer : public BtreeNode {

        public:
            BtreeSequencer(BtreeNode* p_parent, const size_t k_childrenMaxCount);
            explicit BtreeSequencer(const size_t k_childrenMaxCount);

            State evaluate(void* data) const override;
        };
	}
}