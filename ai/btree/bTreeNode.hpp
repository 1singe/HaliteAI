#pragma once

#include "bTreeNode.hpp"

#include <iostream>

namespace ai {
    namespace btree {
        class BtreeNode {

        public:
            enum class State {
                RUNNING,
                SUCCESS,
                FAILURE
            };

            virtual State evaluate(void* data) const = 0;
            size_t addChild(BtreeNode* p_child);

        protected:
            BtreeNode** mpp_children;
            BtreeNode* mp_parent;
            size_t m_childrenCount;
            size_t m_childrenMaxCount;

            virtual ~BtreeNode();
            explicit BtreeNode(const size_t k_childrenMaxCount);
            BtreeNode(BtreeNode* p_parent, const size_t k_childrenMaxCount);

        private:
            void initChildrenArray(const size_t k_childrenMaxCount);

        };
    }
}