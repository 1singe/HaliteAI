#pragma once

#include "bTreeNode.hpp"

namespace ai {
	namespace btree {
        class BtreeLeaf : public BtreeNode {

        public:
            typedef BtreeNode::State(*EVALUATE_CBK)(void* data);

        protected:
            EVALUATE_CBK  m_evaluateCBK;

        public:
            BtreeLeaf(BtreeNode* p_parent, EVALUATE_CBK evaluateCBK);

            State evaluate(void* data) const override;
        };
	}
}