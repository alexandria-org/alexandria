
#include "ApiResponse.h"

using namespace Aws::Utils::Json;

ApiResponse::ApiResponse() {
	m_status = "success";
}

ApiResponse::~ApiResponse() {

}

void ApiResponse::set_results(vector<SearchResult> &results) {
	m_results = results;
}

void ApiResponse::set_debug(const string &variable_name, size_t value) {
	m_debug_variables[variable_name] = value;
}

void ApiResponse::set_failure(const string &reason) {
	m_status = "error";
	m_failure_reason = reason;
}

vector<SearchResult> ApiResponse::results() const {
	return m_results;
}

string ApiResponse::status() const {
	return m_status;
}

string ApiResponse::json() const {

	JsonValue message("{}");

	Aws::Utils::Array<Aws::Utils::Json::JsonValue> result_array(m_results.size());

	size_t idx = 0;
	for (const SearchResult &result : m_results) {
		JsonValue json_result;
		JsonValue string;
		JsonValue json_number;
		json_result.WithObject("url", string.AsString(result.url()));
		json_result.WithObject("title", string.AsString(clean_string(result.title())));
		json_result.WithObject("snippet", string.AsString(clean_string(result.snippet())));
		json_result.WithObject("score", json_number.AsDouble(result.score()));
		result_array[idx] = json_result;
		idx++;
	}

	JsonValue json_results, json_number;
	json_results.AsArray(result_array);
	message.WithObject("results", json_results);

	// Write some profiling data.
	JsonValue debug("{}");
	for (const auto &iter : m_debug_variables) {
		debug.WithObject(iter.first, json_number.AsInteger(iter.second));
	}
	message.WithObject("debug", debug);

	JsonValue response, json_string;

	response.WithObject("status", json_string.AsString("success"));
	response.WithObject("message", message);
	return response.View().WriteReadable();
}