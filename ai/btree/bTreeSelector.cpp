#include "bTreeSelector.hpp"

ai::btree::BtreeSelector::BtreeSelector(btree::BtreeNode* p_parent, const size_t k_childrenMaxCount)
    : BtreeNode(p_parent, k_childrenMaxCount) {}

ai::btree::BtreeSelector::BtreeSelector(const size_t k_childrenMaxCount)
    : BtreeNode(k_childrenMaxCount) {}

ai::btree::BtreeNode::State ai::btree::BtreeSelector::evaluate(void* data) const {
    for (size_t iChild = 0; iChild < m_childrenCount; ++iChild) {
        BtreeNode::State  childState = mpp_children[iChild]->evaluate(data);
        if (childState == BtreeNode::State::SUCCESS) {
            return BtreeNode::State::SUCCESS;
        }
        if (childState == BtreeNode::State::RUNNING) {
            return BtreeNode::State::RUNNING;
        }
    }
    return BtreeNode::State::FAILURE;
}
