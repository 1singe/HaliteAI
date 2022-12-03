#include "bTreeNode.hpp"

size_t ai::btree::BtreeNode::addChild(BtreeNode* p_child) {
    if (m_childrenCount >= m_childrenMaxCount)
        std::cerr << "Tries to add a child beyond limited capacity" << std::endl;
    else {
        mpp_children[m_childrenCount++] = p_child;
        p_child->mp_parent = this;
    }
    return m_childrenCount;
}

ai::btree::BtreeNode::~BtreeNode() {
    delete[] mpp_children;
}

ai::btree::BtreeNode::BtreeNode(const size_t k_childrenMaxCount)
    : mp_parent(0), m_childrenMaxCount(k_childrenMaxCount), m_childrenCount(0), mpp_children(nullptr) {
    initChildrenArray(k_childrenMaxCount);
}

ai::btree::BtreeNode::BtreeNode(btree::BtreeNode* p_parent, const size_t k_childrenMaxCount) :
    mp_parent(p_parent), m_childrenMaxCount(k_childrenMaxCount), m_childrenCount(0), mpp_children(nullptr) {
    initChildrenArray(k_childrenMaxCount);
}

void ai::btree::BtreeNode::initChildrenArray(const size_t k_childrenMaxCount) {
    mpp_children = new BtreeNode * [k_childrenMaxCount];
}