
#pragma once

#include <iostream>
#include "fcgio.h"
#include "config.h"
#include "parser/URL.h"

#include "post_processor/PostProcessor.h"
#include "api/ApiResponse.h"

#include "hash_table/HashTable.h"

#include "full_text/FullText.h"
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextRecord.h"
#include "full_text/SearchMetric.h"
#include "search_engine/SearchAllocation.h"

#include "api/Api.h"

#include "link_index/LinkFullTextRecord.h"

#include "system/Logger.h"

namespace Worker {

	struct Worker {

		int socket_id;
		int thread_id;

	};

	void output_response(FCGX_Request &request, stringstream &response);
	void *run_worker(void *data);

}

