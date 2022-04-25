
cmake_minimum_required(VERSION 3.5)
set(CMAKE_C_COMPILER /usr/bin/gcc-10)
set(CMAKE_CXX_COMPILER /usr/bin/g++-10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(THREADS_PREFER_PTHREAD_FLAG ON)
project(alexandria LANGUAGES CXX)

add_definitions(-Wfatal-errors)
add_compile_options(-Wall -Werror)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_subdirectory("deps/leveldb")
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
	"src/url_link/link_counter.cpp"

	"src/domain_link/indexer.cpp"

	"src/hash_table/hash_table.cpp"
	"src/hash_table/hash_table_shard.cpp"
	"src/hash_table/hash_table_shard_builder.cpp"
	"src/hash_table/builder.cpp"

	"src/hash_table_helper/hash_table_helper.cpp"

	"src/post_processor/post_processor.cpp"
	
	"src/parser/parser.cpp"
	"src/parser/entities.cpp"
	"src/parser/html_link.cpp"
	"src/parser/html_parser.cpp"
	"src/parser/unicode.cpp"

	"src/URL.cpp"

	"src/parser/cc_parser.cpp"

	"src/warc/warc.cpp"

	"src/url_store/url_store.cpp"
	"src/url_store/url_data.cpp"
	"src/url_store/domain_data.cpp"
	"src/url_store/robots_data.cpp"
	
	"src/profiler/profiler.cpp"

	"src/logger/logger.cpp"

	"src/utils/thread_pool.cpp"

	"src/memory/memory.cpp"
	"src/memory/debugger.cpp"

	"src/config.cpp"

	"src/key_value_store.cpp"

	"src/algorithm/algorithm.cpp"
	"src/algorithm/intersection.cpp"
	"src/algorithm/sort.cpp"
	"src/algorithm/hyper_ball.cpp"
	"src/algorithm/hash.cpp"
	"src/algorithm/hyper_log_log.cpp"

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
	"src/indexer/snippet.cpp"
	"src/indexer/index_tree.cpp"
	"src/indexer/console.cpp"
	"src/indexer/merger.cpp"
	"src/indexer/score_builder.cpp"
	"src/indexer/index_reader.cpp"

	"src/domain_stats/domain_stats.cpp"

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
	"tests/test_sharded_index_builder.cpp"
	"tests/test_counted_index_builder.cpp"
	"tests/test_index_reader.cpp"
	"tests/test_top_k.cpp"
	"tests/test_sum_sorted.cpp"
	"tests/hyper_log_log.cpp"
	"tests/memory.cpp"
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

target_compile_definitions(run_tests PUBLIC CC_TESTING)
target_compile_definitions(run_tests PUBLIC FT_NUM_SHARDS=16)
target_compile_definitions(run_tests PUBLIC HT_NUM_SHARDS=16)
target_compile_definitions(run_tests PUBLIC FILE_SERVER="http://127.0.0.1")
target_compile_definitions(run_tests PUBLIC COMPILE_WITH_LINK_INDEX)

target_compile_options(run_tests PUBLIC -Wall -Werror)
target_compile_options(server PUBLIC -Wall -Werror)
target_compile_options(scraper PUBLIC -Wall -Werror)
target_compile_options(indexer PUBLIC -Wall -Werror)

target_link_libraries(run_tests PUBLIC
	${FCGI_LIBRARY}
	${FCGI_LIBRARYCPP}
	${CURL_LIBRARIES}
	${Boost_LIBRARIES} ZLIB::ZLIB Threads::Threads leveldb absl::strings absl::numeric roaring::roaring)
target_link_libraries(server PUBLIC
	${FCGI_LIBRARY}
	${FCGI_LIBRARYCPP}
	${CURL_LIBRARIES}
	${Boost_LIBRARIES} ZLIB::ZLIB Threads::Threads leveldb absl::strings absl::numeric roaring::roaring)
target_link_libraries(scraper PUBLIC
	${FCGI_LIBRARY}
	${FCGI_LIBRARYCPP}
	${CURL_LIBRARIES}
	${Boost_LIBRARIES} ZLIB::ZLIB Threads::Threads leveldb absl::strings absl::numeric roaring::roaring)
target_link_libraries(indexer PUBLIC
	${FCGI_LIBRARY}
	${FCGI_LIBRARYCPP}
	${CURL_LIBRARIES}
	${Boost_LIBRARIES} ZLIB::ZLIB Threads::Threads leveldb absl::strings absl::numeric roaring::roaring)

