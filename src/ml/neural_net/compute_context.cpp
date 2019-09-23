/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/compute_context.hpp>

#include <algorithm>
#include <map>

namespace turi {
namespace neural_net {

namespace {

std::multimap<int, compute_context::registration*> &get_registry() {
  static auto *registry =
      new std::multimap<int, compute_context::registration*>;
  return *registry;
}

}  // namespace

compute_context::registration::registration(int priority, factory factory_fn,
                                            factory tf_factory_fn)
    : priority_(priority),
      factory_fn_(std::move(factory_fn)),
      tf_factory_fn_(std::move(tf_factory_fn)) {
  // No mutex is required if this is only used at static init time...
  get_registry().emplace(priority, this);
}

compute_context::registration::~registration() {
  // No mutex is required if this is only used at static init time...
  std::pair<const int, compute_context::registration *> needle(priority_, this);
  auto it = std::find(get_registry().begin(), get_registry().end(),
                      needle);
  if (it != get_registry().end()) {
    get_registry().erase(it);
  }
}

// static
std::unique_ptr<compute_context> compute_context::create_tf() {
  // Return the tensorflow compute context only.
  std::unique_ptr<compute_context> result;
  auto it = get_registry().begin();
  while (result == nullptr && it != get_registry().end()) {
    result = it->second->create_tensorflow_context();
    ++it;
  }
  return result;
}

// static
std::unique_ptr<compute_context> compute_context::create() {
  // Return the first compute context created by any factory function, in
  // ascending order by priority.
  std::unique_ptr<compute_context> result;
  auto it = get_registry().begin();
  while (result == nullptr && it != get_registry().end()) {
    result = it->second->create_context();
    ++it;
  }
  return result;
}

compute_context::~compute_context() = default;

}  // namespace neural_net
}  // namespace turi
