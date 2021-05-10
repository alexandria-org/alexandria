
cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
project(cc_parser LANGUAGES CXX)

find_package(aws-lambda-runtime REQUIRED)
find_package(AWSSDK COMPONENTS s3 transfer)
find_package(Boost REQUIRED COMPONENTS system iostreams filesystem)
find_package(ZLIB)

include_directories(src/)

add_executable(${PROJECT_NAME} "src/main.cpp" "src/common.cpp" "src/entities.cpp" "src/HtmlParser.cpp" "src/Link.cpp" "src/Profiler.cpp")
add_executable(cc_indexer "src/indexer.cpp" "src/common.cpp" "src/TsvFile.cpp" "src/BasicUrlData.cpp" "src/BasicLinkData.cpp"
	"src/BasicData.cpp" "src/URL.cpp" "src/Stopwords.cpp" "src/CCUrlIndexer.cpp" "src/BasicIndexer.cpp" "src/CCLinkIndexer.cpp"  "src/TsvFileS3.cpp" "src/Dictionary.cpp"
	"src/DictionaryRow.cpp" "src/SubSystem.cpp" "src/Profiler.cpp" "src/CCFullTextIndexer.cpp")
add_executable(cc_api "src/api.cpp" "src/common.cpp" "src/CCApi.cpp" "src/Stopwords.cpp" "src/SearchResult.cpp"
	"src/URL.cpp" "src/Profiler.cpp")
add_executable(cc_full_text "src/full_text.cpp" "src/common.cpp" "src/TsvFile.cpp" "src/BasicUrlData.cpp" "src/BasicLinkData.cpp"
"src/BasicData.cpp" "src/URL.cpp" "src/Stopwords.cpp" "src/CCUrlIndexer.cpp" "src/BasicIndexer.cpp" "src/CCLinkIndexer.cpp"  "src/TsvFileS3.cpp" "src/Dictionary.cpp"
"src/DictionaryRow.cpp" "src/SubSystem.cpp" "src/Profiler.cpp" "src/CCFullTextIndexer.cpp")
add_executable(run_tests "tests/test.cpp" "src/common.cpp" "tests/test1.cpp" "tests/test2.cpp" "tests/test3.cpp" "src/entities.cpp" "src/HtmlParser.cpp"
	"src/Link.cpp" "src/TsvFile.cpp" "src/BasicUrlData.cpp" "src/BasicLinkData.cpp" "src/BasicData.cpp" "src/URL.cpp"
	"src/Stopwords.cpp" "src/CCUrlIndexer.cpp" "src/BasicIndexer.cpp" "src/CCFullTextIndexer.cpp" "src/TsvFileS3.cpp" "src/Dictionary.cpp" "src/DictionaryRow.cpp"
	"src/SubSystem.cpp" "src/Profiler.cpp")

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

