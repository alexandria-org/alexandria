

#include "test4.h"
#include "common.h"
#include "CCApi.h"

using namespace std;

/*
 * Test Search API
 */
int test4_1(void) {
	int ok = 1;

	Aws::SDKOptions options;
	Aws::InitAPI(options);
	Aws::S3::S3Client s3_client = get_s3_client();

	CCApi api(s3_client);
	ApiResponse response = api.query("låna");

	Aws::ShutdownAPI(options);

	return ok;
}