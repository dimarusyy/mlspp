#pragma once

#include <cstdint>
#include <tls/tls_syntax.h>
#include <vector>
#include <type_traits>

// The below functions provide the index calculus for the tree
// structures used in MLS.  They are premised on a "flat"
// representation of a balanced binary tree.  Leaf nodes are
// even-numbered nodes, with the n-th leaf at 2*n.  Intermediate
// nodes are held in odd-numbered nodes.  For example, a 11-element
// tree has the following structure:
//
//                                              X
//                      X
//          X                       X                       X
//    X           X           X           X           X
// X     X     X     X     X     X     X     X     X     X     X
// 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f 10 11 12 13 14
//
// This allows us to compute relationships between tree nodes simply
// by manipulating indices, rather than having to maintain
// complicated structures in memory, even for partial trees.  (The
// storage for a tree can just be a map[int]Node dictionary or an
// array.)  The basic rule is that the high-order bits of parent and
// child nodes have the following relation:
//
//    01x = <00x, 10x>

namespace mls {

// Index types go in the overall namespace
// XXX(rlb@ipv.sx): Seems like this stuff can probably get
// simplified down a fair bit.

namespace traits {
template<typename T>
struct is_unsigned
{
  constexpr static bool value =
    std::is_integral<T>::value &&
    !std::is_same<T, bool>::value &&
#if ((defined(_MSVC_LANG) && _MSVC_LANG > 201703L) || __cplusplus > 201703L)
    !std::is_same<T, char8_t>::value &&
#endif
    !std::is_same<T, char16_t>::value &&
    !std::is_same<T, char32_t>::value;
};
}

template <typename T>
struct TreeIndexBase
{
  typedef T value_type;
  value_type val;

  TreeIndexBase()
    : val(0)
  {}

  template<
    typename U,
    typename std::enable_if<traits::is_unsigned<U>::value>::type* = nullptr>
  explicit TreeIndexBase(U val_in)
    : val(static_cast<value_type>(val_in))
  {}

  TLS_SERIALIZABLE(val)
};

typedef TreeIndexBase<size_t> TreeIndex;

// forward declaration
struct NodeCount;

struct LeafCount : public TreeIndex
{
  using TreeIndex::TreeIndex;
  explicit LeafCount(const NodeCount w);
};

struct NodeCount : public TreeIndex
{
  using TreeIndex::TreeIndex;
  explicit NodeCount(const LeafCount n);
};

struct LeafIndex : public TreeIndex
{
  using TreeIndex::TreeIndex;
  bool operator<(const LeafIndex other) const { return val < other.val; }
  bool operator<(const LeafCount other) const { return val < other.val; }
};

struct NodeIndex : public TreeIndex
{
  using TreeIndex::TreeIndex;
  explicit NodeIndex(const LeafIndex x)
    : TreeIndex(2 * x.val)
  {}

  bool operator<(const NodeIndex other) const { return val < other.val; }
};

// Internal namespace to keep these generic names clean
namespace tree_math {

NodeIndex::value_type
level(NodeIndex x);

// Node relationships
NodeIndex
root(NodeCount w);

NodeIndex
left(NodeIndex x);

NodeIndex
right(NodeIndex x, NodeCount w);

NodeIndex
parent(NodeIndex x, NodeCount w);

NodeIndex
sibling(NodeIndex x, NodeCount w);

std::vector<NodeIndex>
dirpath(NodeIndex x, NodeCount w);

std::vector<NodeIndex>
copath(NodeIndex x, NodeCount w);

bool
in_path(NodeIndex x, NodeIndex y);

NodeIndex
ancestor(LeafIndex l, LeafIndex r);

} // namespace tree_math
} // namespace mls
