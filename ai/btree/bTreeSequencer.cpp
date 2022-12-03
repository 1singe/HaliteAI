#include "bTreeSequencer.hpp"

ai::btree::BtreeSequencer::BtreeSequencer(btree::BtreeNode* p_parent, const size_t k_childrenMaxCount)
    : BtreeNode(p_parent, k_childrenMaxCount) {}

ai::btree::BtreeSequencer::BtreeSequencer(const size_t k_childrenMaxCount)
    : BtreeNode(k_childrenMaxCount) {}

ai::btree::BtreeNode::State ai::btree::BtreeSequencer::evaluate(void* data) const {
    for (size_t iChild = 0; iChild < m_childrenCount; ++iChild) {
        BtreeNode::State  childState = mpp_children[iChild]->evaluate(data);
        if (childState == BtreeNode::State::FAILURE) {
            return BtreeNode::State::FAILURE;
        }
        if (childState == BtreeNode::State::RUNNING) {
            return BtreeNode::State::RUNNING;
        }
    }
    return BtreeNode::State::SUCCESS;
}