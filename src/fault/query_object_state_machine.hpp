/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FAULT_QUERY_OBJECT_STATE_MACHINE_HPP
#define FAULT_QUERY_OBJECT_STATE_MACHINE_HPP
namespace libfault {

/*
 This is the state machine describing the states of a running query object.

 Masters are simple.
 masters are created in the MASTER_ACTIVE state
 MASTER_ACTIVE  --- (on deactivation)  --->  MASTER_INACTIVE


 Replicas are somewhat messier.
 Replicas are created in the REPLICA_INACTIVE state.
 And it sends a message to the current master to ask for a snapshot.

 REPLICA_INACTIVE -- (on receive snapshot) --> REPLICA_ACTIVE

 At any point if

 */
enum object_state {
  MASTER_INACTIVE,
  MASTER_ACTIVE

};

} // fault
#endif
