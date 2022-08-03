
cmake_minimum_required(VERSION 3.5)
set(CMAKE_C_COMPILER /usr/bin/gcc-10)
set(CMAKE_CXX_COMPILER /usr/bin/g++-10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(THREADS_PREFER_PTHREAD_FLAG ON)
project(alexandria LANGUAGES CXX)

add_definitions(-Wfatal-errors)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_subdirectory("deps/abseil-cpp")

find_package(roaring REQUIRED)
find_package(Threads REQUIRED)
FIND_PACKAGE(CURL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system iostreams filesystem unit_test_framework)
find_package(ZLIB)
find_package(fcgi)

include_directories(src/)
include_directories(deps/)
include_directories(tests/)

set(SRC_CLASSES
	
	"src/api/api.cpp"
	"src/api/api_response.cpp"
	"src/api/api_status_response.cpp"
	"src/api/domain_link_result.cpp"
	"src/api/result_with_snippet.cpp"

	"src/worker/worker.cpp"

	"src/file/file.cpp"
	"src/file/archive.cpp"
	"src/file/tsv_file.cpp"
	"src/file/gz_tsv_file.cpp"
	"src/file/tsv_file_remote.cpp"
	"src/file/tsv_row.cpp"

	"src/transfer/transfer.cpp"

	"src/full_text/full_text_indexer.cpp"
	"src/full_text/full_text_indexer_runner.cpp"
	"src/full_text/url_to_domain.cpp"
	"src/full_text/full_text.cpp"

	"src/search_engine/search_engine.cpp"

	"src/search_allocation/search_allocation.cpp"

	"src/url_link/link.cpp"
	"src/url_link/indexer.cpp"
	"src/url_link/indexer_runner.cpp"

	"src/domain_link/indexer.cpp"

	"src/hash_table/hash_table.cpp"
	"src/hash_table/hash_table_shard.cpp"
	"src/hash_table/hash_table_shard_builder.cpp"
	"src/hash_table/builder.cpp"

	"src/hash_table2/hash_table.cpp"
	"src/hash_table2/hash_table_shard.cpp"
	"src/hash_table2/hash_table_shard_builder.cpp"
	"src/hash_table2/builder.cpp"

	"src/hash_table_helper/hash_table_helper.cpp"

	"src/post_processor/post_processor.cpp"
	
	"src/parser/parser.cpp"
	"src/parser/entities.cpp"
	"src/parser/html_link.cpp"
	"src/parser/html_parser.cpp"
	"src/parser/unicode.cpp"
	"src/parser/cc_parser.cpp"

	"src/downloader/warc_downloader.cpp"
	"src/downloader/merge_downloader.cpp"

	"src/URL.cpp"

	"src/warc/warc.cpp"

	"src/profiler/profiler.cpp"

	"src/logger/logger.cpp"

	"src/utils/thread_pool.cpp"

	"src/memory/memory.cpp"
	"src/memory/debugger.cpp"

	"src/config.cpp"

	"src/algorithm/algorithm.cpp"
	"src/algorithm/intersection.cpp"
	"src/algorithm/sort.cpp"
	"src/algorithm/hash.cpp"
	"src/algorithm/hyper_log_log.cpp"
	"src/algorithm/bloom_filter.cpp"

	"src/tools/splitter.cpp"
	"src/tools/find_links.cpp"
	"src/tools/counter.cpp"
	"src/tools/download.cpp"
	"src/tools/calculate_harmonic.cpp"
	"src/tools/generate_url_lists.cpp"

	"src/cluster/document.cpp"
	"src/scraper/scraper.cpp"
	"src/scraper/scraper_store.cpp"

	"src/indexer/level.cpp"
	"src/indexer/domain_level.cpp"
	"src/indexer/url_level.cpp"
	"src/indexer/snippet.cpp"
	"src/indexer/index_manager.cpp"
	"src/indexer/console.cpp"
	"src/indexer/merger.cpp"
	"src/indexer/score_builder.cpp"
	"src/indexer/index_reader.cpp"
	"src/indexer/index_utils.cpp"

	"src/server/search_server.cpp"
	"src/server/url_server.cpp"

	"src/http/server.cpp"
	"src/http/request.cpp"

	"src/domain_stats/domain_stats.cpp"
	"src/debug.cpp"

	"deps/robots.cc"
)

set(SRC_COMMON
	"src/common/dictionary.cpp"
	"src/common/system.cpp"
	"src/common/sub_system.cpp"
	"src/common/datetime.cpp"
	"src/common/dictionary_row.cpp"
	"src/text/stopwords.cpp"
	"src/text/text.cpp"
)

set(SRC_TESTS
	"tests/test_hyper_log_log.cpp"
	"tests/test_memory.cpp"
	"tests/test_algorithm.cpp"
	"tests/test_api.cpp"
	"tests/test_bloom_filter.cpp"
	"tests/test_cc_parser.cpp"
	"tests/test_config.conf"
	"tests/test_config2.conf"
	"tests/test_configuration.cpp"
	"tests/test_counted_index_builder.cpp"
	"tests/test_datetime.h"
	"tests/test_deduplication.cpp"
	"tests/test_file.cpp"
	"tests/test_full_text.cpp"
	"tests/test_hash.cpp"
	"tests/test_hash_table.cpp"
	"tests/test_html_parser.cpp"
	"tests/test_hyper_ball.cpp"
	"tests/test_index_builder.cpp"
	"tests/test_index_iteration.cpp"
	"tests/test_index_reader.cpp"
	"tests/test_logger.cpp"
	"tests/test_n_gram.cpp"
	"tests/test_robot_parser.cpp"
	"tests/test_scraper.cpp"
	"tests/test_search_allocation.cpp"
	"tests/test_search_engine.cpp"
	"tests/test_sections.cpp"
	"tests/test_shard_builder.cpp"
	"tests/test_sharded_index_builder.cpp"
	"tests/test_sort.cpp"
	"tests/test_sub_system.cpp"
	"tests/test_sum_sorted.cpp"
	"tests/test_text.cpp"
	"tests/test_thread_pool.cpp"
	"tests/test_top_k.cpp"
	"tests/test_unicode.cpp"
	"tests/test_url.cpp"
	"tests/test_url_record.cpp"

	# This overloads the new/delete operators to keep track of memory, slows things down a lot.
	"src/memory/overload.cpp"
)

add_executable(run_tests
	"tests/main.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
	${SRC_TESTS}
)
add_executable(server
	"src/server.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
)
add_executable(scraper
	"src/scraper.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
)
add_executable(indexer
	"src/indexer.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
)
add_executable(alexandria
	"src/alexandria.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
)

target_compile_definitions(run_tests PUBLIC IS_TEST)
target_compile_definitions(run_tests PUBLIC FT_NUM_SHARDS=16)
target_compile_definitions(run_tests PUBLIC HT_NUM_SHARDS=16)
target_compile_definitions(run_tests PUBLIC FILE_SERVER="http://127.0.0.1")
target_compile_definitions(run_tests PUBLIC COMPILE_WITH_LINK_INDEX)

target_compile_options(run_tests PUBLIC -Wall -Werror)
target_compile_options(server PUBLIC -Wall -Werror)
target_compile_options(scraper PUBLIC -Wall -Werror)
target_compile_options(indexer PUBLIC -Wall -Werror)
target_compile_options(alexandria PUBLIC -Wall -Werror)

target_link_libraries(run_tests PUBLIC
	${FCGI_LIBRARY}
	${FCGI_LIBRARYCPP}
	${CURL_LIBRARIES}
	${Boost_LIBRARIES} ZLIB::ZLIB Threads::Threads absl::strings absl::numeric roaring::roaring)
target_link_libraries(server PUBLIC
	${FCGI_LIBRARY}
	${FCGI_LIBRARYCPP}
	${CURL_LIBRARIES}
	${Boost_LIBRARIES} ZLIB::ZLIB Threads::Threads absl::strings absl::numeric roaring::roaring)
target_link_libraries(scraper PUBLIC
	${FCGI_LIBRARY}
	${FCGI_LIBRARYCPP}
	${CURL_LIBRARIES}
	${Boost_LIBRARIES} ZLIB::ZLIB Threads::Threads absl::strings absl::numeric roaring::roaring)
target_link_libraries(indexer PUBLIC
	${FCGI_LIBRARY}
	${FCGI_LIBRARYCPP}
	${CURL_LIBRARIES}
	${Boost_LIBRARIES} ZLIB::ZLIB Threads::Threads absl::strings absl::numeric roaring::roaring)
target_link_libraries(alexandria PUBLIC
	${FCGI_LIBRARY}
	${FCGI_LIBRARYCPP}
	${CURL_LIBRARIES}
	${Boost_LIBRARIES} ZLIB::ZLIB Threads::Threads absl::strings absl::numeric roaring::roaring)
