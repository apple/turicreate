/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FAULT_MESSAGE_FLAGS_HPP
#define FAULT_MESSAGE_FLAGS_HPP

// If this is a query. Mutually exclusive with QO_MESSAGE_FLAG_UPDATE
#define QO_MESSAGE_FLAG_QUERY  1
// If this is an update. Mutually exclusive with QO_MESSAGE_FLAG_QUERY
#define QO_MESSAGE_FLAG_UPDATE 2
// if a reply is expected.
#define QO_MESSAGE_FLAG_NOREPLY 4              // overrideable

// if the message can be sent to any master/slave
#define QO_MESSAGE_FLAG_ANY_TARGET   8



// If this bit is set, the message
// should reply with the complete serialized contents
// of the object.
#define QO_MESSAGE_FLAG_GET_SERIALIZED_CONTENTS 16

#endif
