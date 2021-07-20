
#include "ApiResponse.h"

using namespace Aws::Utils::Json;

ApiResponse::ApiResponse(vector<ResultWithSnippet> &results, size_t total_results, double profile) {

	JsonValue message("{}");

	Aws::Utils::Array<Aws::Utils::Json::JsonValue> result_array(results.size());

	size_t idx = 0;
	for (const ResultWithSnippet &result : results) {
		JsonValue json_result;
		JsonValue string;
		JsonValue json_number;

		//cout << result.title() << endl;
		//cout << result.snippet() << endl;

		json_result.WithObject("url", string.AsString(result.url().str()));
		json_result.WithObject("title", string.AsString(Unicode::encode(result.title())));
		json_result.WithObject("snippet", string.AsString(Unicode::encode(result.snippet())));
		json_result.WithObject("score", json_number.AsDouble(result.score()));
		result_array[idx] = json_result;
		idx++;
	}

	JsonValue json_results, json_string, json_number;
	json_results.AsArray(result_array);
	message.WithObject("status", json_string.AsString("success"));
	message.WithObject("time_ms", json_string.AsDouble(profile));
	message.WithObject("total_found", json_number.AsInt64(total_results));
	message.WithObject("results", json_results);

	//m_response = message.View().WriteCompact();
	m_response = message.View().WriteReadable();
}

ApiResponse::~ApiResponse() {

}

ostream &operator<<(ostream &os, const ApiResponse &api_response) {
	os << api_response.m_response;
	return os;
}

