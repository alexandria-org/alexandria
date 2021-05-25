

#include "test4.h"
#include "common/common.h"
#include "api/CCApi.h"

using namespace std;
using namespace Aws::Utils::Json;

/*
 * Test Search API
 */
int test4_1(void) {
	int ok = 1;
	return ok;

	Aws::SDKOptions options;
	Aws::InitAPI(options);
	Aws::S3::S3Client s3_client = get_s3_client();

	CCApi api(s3_client);
	//ApiResponse response = api.query("låna pengar");

	//cout << response.json() << endl;

	//LinkResult("col1	col2	col3	col4	col5	col6	col7	col8	col9");

	ApiResponse response2 = api.query("cnn");
	string response_body = response2.json();

	JsonValue response_json;

	response_json.WithObject("statusCode", JsonValue().AsInteger(200));
	response_json.WithObject("headers", JsonValue().WithObject("Access-Control-Allow-Origin", JsonValue().AsString("*")));
	response_json.WithObject("body", JsonValue().AsString(response_body));
	response_json.WithObject("isBase64Encoded", JsonValue().AsBool(true));

	//cout << "invocation_response: " << response_json.View().WriteReadable() << endl;

	Aws::ShutdownAPI(options);

	return ok;
}
