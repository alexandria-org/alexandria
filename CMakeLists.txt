
cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
project(cc_parser LANGUAGES CXX)

find_package(aws-lambda-runtime REQUIRED)
find_package(AWSSDK COMPONENTS s3 transfer)
find_package(Boost REQUIRED COMPONENTS system iostreams filesystem)
find_package(ZLIB)

include_directories(include/)
include_directories(tests/)

set(SRC_CLASSES
	"src/classes/ApiResponse.cpp"
	"src/classes/CCApi.cpp"
	"src/classes/CCFullTextIndexer.cpp"
	"src/classes/CCLinkIndex.cpp"
	"src/classes/CCIndexRunner.cpp"
	"src/classes/CCUrlIndex.cpp"
	"src/classes/Dictionary.cpp"
	"src/classes/DictionaryRow.cpp"
	"src/classes/HtmlParser.cpp"
	"src/classes/Link.cpp"
	"src/classes/Profiler.cpp"
	"src/classes/SearchResult.cpp"
	"src/classes/Stopwords.cpp"
	"src/classes/SubSystem.cpp"
	"src/classes/TsvFile.cpp"
	"src/classes/TsvFileS3.cpp"
	"src/classes/TsvRow.cpp"
	"src/classes/URL.cpp"
	"src/classes/LinkResult.cpp"

	"src/abstract/BasicData.cpp"
	"src/abstract/BasicIndexer.cpp"
	"src/abstract/BasicLinkData.cpp"
	"src/abstract/BasicUrlData.cpp"
	"src/abstract/TextBase.cpp"
)

set(SRC_COMMON
	"src/common.cpp"
	"src/entities.cpp"
)

set(SRC_TESTS
	"tests/test1.cpp"
	"tests/test2.cpp"
	"tests/test3.cpp"
	"tests/test4.cpp"
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

