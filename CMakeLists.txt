
cmake_minimum_required(VERSION 3.5)
set(CMAKE_C_COMPILER /usr/bin/gcc-10)
set(CMAKE_CXX_COMPILER /usr/bin/g++-10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
project(cc_parser LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(aws-lambda-runtime REQUIRED)
find_package(AWSSDK COMPONENTS s3 transfer lambda)
find_package(Boost REQUIRED COMPONENTS system iostreams filesystem unit_test_framework)
find_package(ZLIB)
find_package(fcgi)

include_directories(src/)
include_directories(tests/)

set(SRC_CLASSES
	
	"src/api/Api.cpp"
	"src/api/ApiResponse.cpp"
	"src/api/ResultWithSnippet.cpp"
	"src/api/LinkResult.cpp"
	"src/api/DomainLinkResult.cpp"

	"src/file/File.cpp"
	"src/file/Transfer.cpp"
	"src/file/TsvFile.cpp"
	"src/file/TsvFileS3.cpp"
	"src/file/TsvFileRemote.cpp"
	"src/file/TsvRow.cpp"

	"src/full_text/FullTextIndexer.cpp"
	"src/full_text/FullTextIndexerRunner.cpp"
	"src/full_text/UrlToDomain.cpp"
	"src/full_text/FullText.cpp"

	"src/search_engine/SearchEngine.cpp"
	"src/search_engine/SearchAllocation.cpp"

	"src/link_index/Link.cpp"
	"src/link_index/LinkIndexer.cpp"
	"src/link_index/DomainLinkIndexer.cpp"
	"src/link_index/LinkIndexerRunner.cpp"

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
	
	"src/system/Profiler.cpp"
	"src/system/System.cpp"
	"src/system/SubSystem.cpp"
	"src/system/Logger.cpp"

	"src/aws/Lambda.cpp"

	"src/sort/Sort.cpp"

	"src/hash/Hash.cpp"
)

set(SRC_COMMON
	"src/common/common.cpp"
	"src/common/Dictionary.cpp"
	"src/common/DictionaryRow.cpp"
	"src/text/Stopwords.cpp"
	"src/text/Text.cpp"
)

add_executable(cc_parser 
	"src/cc_parser.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
)
add_executable(cc_indexer
	"src/indexer.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
)
add_executable(run_tests
	"tests/main.cpp"
	"config/test_config/config.cpp"
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

target_include_directories(cc_parser PUBLIC ./config)
target_include_directories(cc_indexer PUBLIC ./config)
target_include_directories(run_tests PUBLIC ./config/test_config)
target_include_directories(server PUBLIC ./config)

target_link_libraries(cc_parser PUBLIC
	AWS::aws-lambda-runtime 
	${AWSSDK_LINK_LIBRARIES} ${Boost_LIBRARIES} ZLIB::ZLIB)
target_link_libraries(cc_indexer PUBLIC
	AWS::aws-lambda-runtime 
	${AWSSDK_LINK_LIBRARIES} ${Boost_LIBRARIES} ZLIB::ZLIB)
target_link_libraries(run_tests PUBLIC
	AWS::aws-lambda-runtime 
	${AWSSDK_LINK_LIBRARIES} ${Boost_LIBRARIES} ZLIB::ZLIB)
target_link_libraries(server PUBLIC
	AWS::aws-lambda-runtime 
	${FCGI_LIBRARY}
	${FCGI_LIBRARYCPP}
	${AWSSDK_LINK_LIBRARIES} ${Boost_LIBRARIES} ZLIB::ZLIB)

aws_lambda_package_target(${PROJECT_NAME})
aws_lambda_package_target(cc_indexer)

