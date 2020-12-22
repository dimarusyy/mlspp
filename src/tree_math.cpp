#include "mls/tree_math.h"
#include "mls/common.h"

#include <algorithm>

static const size_t one = 0x01;

namespace mls {

LeafCount::LeafCount(const NodeCount w)
{
  if (w.val == 0) {
    val = 0;
    return;
  }

  if ((w.val & one) == 0) {
    throw InvalidParameterError("Only odd node counts describe trees");
  }

  val = (w.val >> one) + 1;
}

NodeCount::NodeCount(const LeafCount n)
  : TreeIndex(2 * (n.val - 1) + 1)
{}

namespace tree_math {

static size_t
log2(size_t x)
{
  if (x == 0) {
    return 0;
  }

  size_t k = 0;
  while ((x >> k) > 0) {
    k += 1;
  }
  return k - 1;
}

NodeIndex::value_type
level(NodeIndex x)
{
  if ((x.val & one) == 0) {
    return 0;
  }

  NodeIndex::value_type k = 0;
  while (((x.val >> k) & one) == 1) {
    k += 1;
  }
  return k;
}

NodeIndex
root(NodeCount w)
{
  return NodeIndex{ (one << log2(w.val)) - 1 };
}

NodeIndex
left(NodeIndex x)
{
  if (level(x) == 0) {
    return x;
  }

  return NodeIndex{ x.val ^ (one << (level(x) - 1)) };
}

NodeIndex
right(NodeIndex x, NodeCount w)
{
  if (level(x) == 0) {
    return x;
  }

  NodeIndex r{ x.val ^ (size_t(0x03) << (level(x) - 1)) };
  while (r.val >= w.val) {
    r = left(r);
  }
  return r;
}

static NodeIndex
parent_step(NodeIndex x)
{
  auto k = level(x);
  return NodeIndex{ (x.val | (one << k)) & ~(one << (k + 1)) };
}

NodeIndex
parent(NodeIndex x, NodeCount w)
{
  if (x == root(w)) {
    return x;
  }

  auto p = parent_step(x);
  while (p.val >= w.val) {
    p = parent_step(p);
  }
  return p;
}

NodeIndex
sibling(NodeIndex x, NodeCount w)
{
  auto p = parent(x, w);
  if (x.val < p.val) {
    return right(p, w);
  }

  if (x.val > p.val) {
    return left(p);
  }

  // root's sibling is itself
  return p;
}

std::vector<NodeIndex>
dirpath(NodeIndex x, NodeCount w)
{
  std::vector<NodeIndex> d;

  auto p = parent(x, w);
  auto r = root(w);
  while (p.val != r.val) {
    d.push_back(p);
    p = parent(p, w);
  }

  if (x.val != r.val) {
    d.push_back(p);
  }

  return d;
}

std::vector<NodeIndex>
copath(NodeIndex x, NodeCount w)
{
  auto d = dirpath(x, w);
  if (d.empty()) {
    return std::vector<NodeIndex>();
  }

  std::vector<NodeIndex> path;
  path.push_back(x);
  // exclude root
  for (size_t i = 0; i < d.size() - 1; ++i) {
    path.push_back(d[i]);
  }

  std::vector<NodeIndex> c(path.size());
  for (size_t i = 0; i < path.size(); ++i) {
    c[i] = sibling(path[i], w);
  }

  return c;
}

bool
in_path(NodeIndex x, NodeIndex y)
{
  auto lx = level(x);
  auto ly = level(y);
  return lx <= ly && (x.val >> (ly + 1) == y.val >> (ly + 1));
}

// Common ancestor of two leaves
NodeIndex
ancestor(LeafIndex l, LeafIndex r)
{
  auto ln = NodeIndex(l);
  auto rn = NodeIndex(r);
  if (ln == rn) {
    return ln;
  }

  uint8_t k = 0;
  while (ln != rn) {
    ln.val = ln.val >> 1;
    rn.val = rn.val >> 1;
    k += 1;
  }

  NodeIndex::value_type prefix = ln.val << k;
  NodeIndex::value_type stop = (size_t(1) << uint8_t(k - 1));
  return NodeIndex(prefix + (stop - 1));
}

} // namespace tree_math
} // namespace mls
