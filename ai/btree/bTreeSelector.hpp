#pragma once

#include "bTreeNode.hpp"

namespace ai {
    namespace btree {

        class BtreeSelector : public BtreeNode {

        public:
            BtreeSelector(BtreeNode* p_parent, const size_t k_childrenMaxCount);
            explicit BtreeSelector(const size_t k_childrenMaxCount);

            State evaluate(void* data) const override;
        };
    }
}