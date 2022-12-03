#include "bTreeDecorator.hpp"

ai::btree::BtreeDecorator::BtreeDecorator(btree::BtreeNode* p_parent, btree::BtreeDecorator::EVALUATE_CBK evaluateCBK)
    : m_evaluateCBK(evaluateCBK), BtreeNode(p_parent, 1) {}

ai::btree::BtreeNode::State ai::btree::BtreeDecorator::evaluate(void* data) const {
    return m_evaluateCBK(data);
}