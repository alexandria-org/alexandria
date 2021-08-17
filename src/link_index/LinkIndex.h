
#pragma once

#include "full_text/FullTextIndex.h"

#define LI_INDEXER_MAX_CACHE_GB 4
#define LI_NUM_THREADS_INDEXING 48
#define LI_NUM_THREADS_MERGING 16
#define LI_INDEXER_CACHE_BYTES_PER_SHARD ((LI_INDEXER_MAX_CACHE_GB * 1000ul*1000ul*1000ul) / (FT_NUM_SHARDS * LI_NUM_THREADS_INDEXING))
#define LI_INDEXER_MAX_CACHE_SIZE 500

#include <iostream>
#include <vector>

#include "system/SubSystem.h"
#include "system/ThreadPool.h"
#include "LinkFullTextRecord.h"
#include "full_text/FullTextShard.h"
#include "full_text/FullTextResultSet.h"


