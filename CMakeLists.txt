
cmake_minimum_required(VERSION 3.5)
set(CMAKE_C_COMPILER /usr/bin/gcc-10)
set(CMAKE_CXX_COMPILER /usr/bin/g++-10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
project(cc_parser LANGUAGES CXX)

find_package(aws-lambda-runtime REQUIRED)
find_package(AWSSDK COMPONENTS s3 transfer)
find_package(Boost REQUIRED COMPONENTS system iostreams filesystem)
find_package(ZLIB)

include_directories(src/)
include_directories(tests/)

set(SRC_CLASSES
	"src/abstract/BasicData.cpp"
	"src/abstract/BasicIndexer.cpp"
	"src/abstract/BasicLinkData.cpp"
	"src/abstract/BasicUrlData.cpp"
	"src/abstract/TextBase.cpp"
	
	"src/api/ApiResponse.cpp"
	"src/api/CCApi.cpp"
	"src/api/ApiLink.cpp"
	"src/api/LinkSearchResult.cpp"
	"src/api/SearchResult.cpp"

	"src/file/TsvFile.cpp"
	"src/file/TsvFileS3.cpp"
	"src/file/TsvRow.cpp"

	"src/full_text/FullTextIndex.cpp"
	"src/full_text/FullTextShard.cpp"
	"src/full_text/FullTextShardBuilder.cpp"
	"src/full_text/FullTextResult.cpp"
	"src/full_text/FullTextResultSet.cpp"
	"src/full_text/FullTextBucketMessage.cpp"
	"src/full_text/FullTextIndexer.cpp"
	"src/full_text/FullTextIndexerRunner.cpp"
	"src/full_text/AdjustmentList.cpp"

	"src/link_index/LinkIndex.cpp"
	"src/link_index/LinkShard.cpp"
	"src/link_index/LinkShardBuilder.cpp"
	"src/link_index/LinkResult.cpp"
	"src/link_index/LinkResultSet.cpp"
	"src/link_index/LinkIndexer.cpp"
	"src/link_index/LinkIndexerRunner.cpp"

	"src/hash_table/HashTable.cpp"
	"src/hash_table/HashTableShard.cpp"
	"src/hash_table/HashTableShardBuilder.cpp"

	"src/index/CCIndexMerger.cpp"
	"src/index/CCIndexRunner.cpp"
	"src/index/CCLinkIndex.cpp"
	"src/index/CCUrlIndex.cpp"
	
	"src/parser/entities.cpp"
	"src/parser/HtmlParser.cpp"
	"src/parser/Unicode.cpp"
	"src/parser/URL.cpp"
	
	"src/system/Profiler.cpp"
	"src/system/SubSystem.cpp"
	"src/system/Logger.cpp"
)

set(SRC_COMMON
	"src/common/common.cpp"
	"src/common/Dictionary.cpp"
	"src/common/DictionaryRow.cpp"
	"src/common/Stopwords.cpp"
)

set(SRC_TESTS
	"tests/test1.cpp"
	"tests/test2.cpp"
	"tests/test3.cpp"
	"tests/test4.cpp"
	"tests/test5.cpp"
	"tests/test6.cpp"
)

add_executable(cc_parser 
	"src/main.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
)
add_executable(cc_indexer
	"src/indexer.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
)
add_executable(cc_api
	"src/api.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
)
add_executable(cc_full_text
	"src/full_text.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
)
add_executable(run_tests
	"tests/test.cpp"
	${SRC_CLASSES}
	${SRC_COMMON}
	${SRC_TESTS}
)

target_compile_definitions(run_tests PUBLIC CC_TESTING)

target_link_libraries(${PROJECT_NAME} PUBLIC
	AWS::aws-lambda-runtime 
	${AWSSDK_LINK_LIBRARIES} ${Boost_LIBRARIES} ZLIB::ZLIB)
target_link_libraries(cc_indexer PUBLIC
	AWS::aws-lambda-runtime 
	${AWSSDK_LINK_LIBRARIES} ${Boost_LIBRARIES} ZLIB::ZLIB)
target_link_libraries(cc_full_text PUBLIC
	AWS::aws-lambda-runtime 
	${AWSSDK_LINK_LIBRARIES} ${Boost_LIBRARIES} ZLIB::ZLIB)
target_link_libraries(cc_api PUBLIC
	AWS::aws-lambda-runtime 
	${AWSSDK_LINK_LIBRARIES} ${Boost_LIBRARIES} ZLIB::ZLIB)
target_link_libraries(run_tests PUBLIC
	AWS::aws-lambda-runtime 
	${AWSSDK_LINK_LIBRARIES} ${Boost_LIBRARIES} ZLIB::ZLIB)

aws_lambda_package_target(${PROJECT_NAME})
aws_lambda_package_target(cc_indexer)
aws_lambda_package_target(cc_api)

