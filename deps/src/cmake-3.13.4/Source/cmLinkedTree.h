/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLinkedTree_h
#define cmLinkedTree_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <assert.h>
#include <iterator>
#include <vector>

/**
  @brief A adaptor for traversing a tree structure in a vector

  This class is not intended to be wholly generic like a standard library
  container adaptor.  Mostly it exists to facilitate code sharing for the
  needs of the cmState.  For example, the Truncate() method is a specific
  requirement of the cmState.

  An empty cmLinkedTree provides a Root() method, and an Push() method,
  each of which return iterators.  A Tree can be built up by extending
  from the root, and then extending from any other iterator.

  An iterator resulting from this tree construction can be
  forward-only-iterated toward the root.  Extending the tree never
  invalidates existing iterators.
 */
template <typename T>
class cmLinkedTree
{
  typedef typename std::vector<T>::size_type PositionType;
  typedef T* PointerType;
  typedef T& ReferenceType;

public:
  class iterator : public std::iterator<std::forward_iterator_tag, T>
  {
    friend class cmLinkedTree;
    cmLinkedTree* Tree;

    // The Position is always 'one past the end'.
    PositionType Position;

    iterator(cmLinkedTree* tree, PositionType pos)
      : Tree(tree)
      , Position(pos)
    {
    }

  public:
    iterator()
      : Tree(nullptr)
      , Position(0)
    {
    }

    void operator++()
    {
      assert(this->Tree);
      assert(this->Tree->UpPositions.size() == this->Tree->Data.size());
      assert(this->Position <= this->Tree->Data.size());
      assert(this->Position > 0);
      this->Position = this->Tree->UpPositions[this->Position - 1];
    }

    PointerType operator->() const
    {
      assert(this->Tree);
      assert(this->Tree->UpPositions.size() == this->Tree->Data.size());
      assert(this->Position <= this->Tree->Data.size());
      assert(this->Position > 0);
      return this->Tree->GetPointer(this->Position - 1);
    }

    PointerType operator->()
    {
      assert(this->Tree);
      assert(this->Tree->UpPositions.size() == this->Tree->Data.size());
      assert(this->Position <= this->Tree->Data.size());
      assert(this->Position > 0);
      return this->Tree->GetPointer(this->Position - 1);
    }

    ReferenceType operator*() const
    {
      assert(this->Tree);
      assert(this->Tree->UpPositions.size() == this->Tree->Data.size());
      assert(this->Position <= this->Tree->Data.size());
      assert(this->Position > 0);
      return this->Tree->GetReference(this->Position - 1);
    }

    ReferenceType operator*()
    {
      assert(this->Tree);
      assert(this->Tree->UpPositions.size() == this->Tree->Data.size());
      assert(this->Position <= this->Tree->Data.size());
      assert(this->Position > 0);
      return this->Tree->GetReference(this->Position - 1);
    }

    bool operator==(iterator other) const
    {
      assert(this->Tree);
      assert(this->Tree->UpPositions.size() == this->Tree->Data.size());
      assert(this->Tree == other.Tree);
      return this->Position == other.Position;
    }

    bool operator!=(iterator other) const
    {
      assert(this->Tree);
      assert(this->Tree->UpPositions.size() == this->Tree->Data.size());
      return !(*this == other);
    }

    bool IsValid() const
    {
      if (!this->Tree) {
        return false;
      }
      return this->Position <= this->Tree->Data.size();
    }

    bool StrictWeakOrdered(iterator other) const
    {
      assert(this->Tree);
      assert(this->Tree == other.Tree);
      return this->Position < other.Position;
    }
  };

  iterator Root() const
  {
    return iterator(const_cast<cmLinkedTree*>(this), 0);
  }

  iterator Push(iterator it) { return Push_impl(it, T()); }

  iterator Push(iterator it, T t) { return Push_impl(it, std::move(t)); }

  bool IsLast(iterator it) { return it.Position == this->Data.size(); }

  iterator Pop(iterator it)
  {
    assert(!this->Data.empty());
    assert(this->UpPositions.size() == this->Data.size());
    bool const isLast = this->IsLast(it);
    ++it;
    // If this is the last entry then no other entry can refer
    // to it so we can drop its storage.
    if (isLast) {
      this->Data.pop_back();
      this->UpPositions.pop_back();
    }
    return it;
  }

  iterator Truncate()
  {
    assert(!this->UpPositions.empty());
    this->UpPositions.erase(this->UpPositions.begin() + 1,
                            this->UpPositions.end());
    assert(!this->Data.empty());
    this->Data.erase(this->Data.begin() + 1, this->Data.end());
    return iterator(this, 1);
  }

  void Clear()
  {
    this->UpPositions.clear();
    this->Data.clear();
  }

private:
  T& GetReference(PositionType pos) { return this->Data[pos]; }

  T* GetPointer(PositionType pos) { return &this->Data[pos]; }

  iterator Push_impl(iterator it, T&& t)
  {
    assert(this->UpPositions.size() == this->Data.size());
    assert(it.Position <= this->UpPositions.size());
    this->UpPositions.push_back(it.Position);
    this->Data.push_back(std::move(t));
    return iterator(this, this->UpPositions.size());
  }

  std::vector<T> Data;
  std::vector<PositionType> UpPositions;
};

#endif
