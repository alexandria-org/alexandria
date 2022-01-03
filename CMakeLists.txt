
cmake_minimum_required(VERSION 3.5)
set(CMAKE_C_COMPILER /usr/bin/gcc-10)
set(CMAKE_CXX_COMPILER /usr/bin/g++-10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(THREADS_PREFER_PTHREAD_FLAG ON)
add_compile_options(-Wall -Werror -Wpedantic)
project(alexandria LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Threads REQUIRED)
FIND_PACKAGE(CURL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system iostreams filesystem unit_test_framework)
find_package(ZLIB)
find_package(fcgi)

include_directories(src/)
include_directories(deps/)
include_directories(tests/)

set(SRC_CLASSES
	
	"src/api/Api.cpp"
	"src/api/ApiResponse.cpp"
	"src/api/ApiStatusResponse.cpp"
	"src/api/ResultWithSnippet.cpp"
	"src/api/LinkResult.cpp"
	"src/api/DomainLinkResult.cpp"
	"src/api/Worker.cpp"

	"src/file/File.cpp"
	"src/file/TsvFile.cpp"
	"src/file/TsvFileRemote.cpp"
	"src/file/TsvRow.cpp"

	"src/transfer/Transfer.cpp"

	"src/full_text/FullTextIndexer.cpp"
	"src/full_text/FullTextIndexerRunner.cpp"
	"src/full_text/UrlToDomain.cpp"
	"src/full_text/FullText.cpp"

	"src/search_engine/SearchEngine.cpp"
	"src/search_engine/SearchAllocation.cpp"

	"src/link/Link.cpp"
	"src/link/Indexer.cpp"
	"src/link/IndexerRunner.cpp"

	"src/domain_link/Indexer.cpp"

	"src/hash_table/HashTable.cpp"
	"src/hash_table/HashTableShard.cpp"
	"src/hash_table/HashTableShardBuilder.cpp"
	"src/hash_table/HashTableHelper.cpp"

	"src/post_processor/PostProcessor.cpp"
	
	"src/parser/Parser.cpp"
	"src/parser/entities.cpp"
	"src/parser/HtmlLink.cpp"
	"src/parser/HtmlParser.cpp"
	"src/parser/Unicode.cpp"
	"src/parser/URL.cpp"
	"src/parser/Warc.cpp"
	"src/parser/cc_parser.cpp"
	
	"src/system/Profiler.cpp"
	"src/system/System.cpp"
	"src/system/SubSystem.cpp"
	"src/system/Logger.cpp"

	"src/sort/Sort.cpp"

	"src/hash/Hash.cpp"

	"src/config.cpp"

	"src/algorithm/Algorithm.cpp"
	"src/algorithm/HyperBall.cpp"

	"src/tools/Splitter.cpp"
	"src/tools/Counter.cpp"
	"src/tools/CalculateHarmonic.cpp"

	"src/cluster/Document.cpp"
)

set(SRC_COMMON
	"src/common/Dictionary.cpp"
	"src/common/DictionaryRow.cpp"
	"src/text/Stopwords.cpp"
	"src/text/Text.cpp"
)

add_executable(tools
	"src/tools.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
)
add_executable(run_tests
	"tests/main.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
)
add_executable(server
	"src/server.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
)

target_compile_definitions(run_tests PUBLIC CC_TESTING)
target_compile_definitions(run_tests PUBLIC FT_NUM_SHARDS=16)
target_compile_definitions(run_tests PUBLIC HT_NUM_SHARDS=16)
target_compile_definitions(run_tests PUBLIC FILE_SERVER="http://127.0.0.1")
target_compile_definitions(run_tests PUBLIC COMPILE_WITH_LINK_INDEX)

target_link_libraries(tools PUBLIC
	${FCGI_LIBRARY}
	${FCGI_LIBRARYCPP}
	${CURL_LIBRARIES}
	${Boost_LIBRARIES} ZLIB::ZLIB Threads::Threads)
target_link_libraries(run_tests PUBLIC
	${FCGI_LIBRARY}
	${FCGI_LIBRARYCPP}
	${CURL_LIBRARIES}
	${Boost_LIBRARIES} ZLIB::ZLIB Threads::Threads)
target_link_libraries(server PUBLIC
	${FCGI_LIBRARY}
	${FCGI_LIBRARYCPP}
	${CURL_LIBRARIES}
	${Boost_LIBRARIES} ZLIB::ZLIB Threads::Threads)

