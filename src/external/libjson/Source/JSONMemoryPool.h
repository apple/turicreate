#ifndef JSON_MEMORY_POOL_H
#define JSON_MEMORY_POOL_H

#ifdef JSON_MEMORY_POOL

#include "../Dependencies/mempool++/mempool.h"

//this macro expands to the number of bytes a pool gets based on block size and number of 32s of the total pool it gets
#define jsonPoolPart(bytes_per_block, thirty_seconds_of_mem) bytes_per_block, ((thirty_seconds_of_mem * JSON_MEMORY_POOL / 32) / bytes_per_block)

#ifdef JSON_PREPARSE
	#define NODEPOOL jsonPoolPart(sizeof(JSONNode), 1)
	#define INTERNALNODEPOOL jsonPoolPart(sizeof(internalJSONNode), 3)
	#define MEMPOOL_1 jsonPoolPart(8, 2)
	#define MEMPOOL_2 jsonPoolPart(16, 2)
	#define MEMPOOL_3 jsonPoolPart(32, 2)
	#define MEMPOOL_4 jsonPoolPart(64, 2)
	#define MEMPOOL_5 jsonPoolPart(128, 3)
	#define MEMPOOL_6 jsonPoolPart(256, 4)
	#define MEMPOOL_7 jsonPoolPart(512, 5)
	#define MEMPOOL_8 jsonPoolPart(4096, 8)
#else
	#define NODEPOOL jsonPoolPart(sizeof(JSONNode), 2)
	#define INTERNALNODEPOOL jsonPoolPart(sizeof(internalJSONNode), 7)
	#define MEMPOOL_1 jsonPoolPart(8, 1)
	#define MEMPOOL_2 jsonPoolPart(16, 1)
	#define MEMPOOL_3 jsonPoolPart(32, 1)
	#define MEMPOOL_4 jsonPoolPart(64, 1)
	#define MEMPOOL_5 jsonPoolPart(128, 3)
	#define MEMPOOL_6 jsonPoolPart(256, 3)
	#define MEMPOOL_7 jsonPoolPart(512, 5)
	#define MEMPOOL_8 jsonPoolPart(4096, 8)
#endif

#endif

#endif

