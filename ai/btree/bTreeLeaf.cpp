#include "bTreeLeaf.hpp"

ai::btree::BtreeLeaf::BtreeLeaf(btree::BtreeNode* p_parent, btree::BtreeLeaf::EVALUATE_CBK evaluateCBK)
    : m_evaluateCBK(evaluateCBK), BtreeNode(p_parent, 0) {}

ai::btree::BtreeNode::State ai::btree::BtreeLeaf::evaluate(void* data) const {
    return m_evaluateCBK(data);
}