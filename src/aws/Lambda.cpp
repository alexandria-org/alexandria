
#include "Lambda.h"
#include <aws/core/Aws.h>
#include <aws/lambda/LambdaClient.h>
#include <aws/lambda/model/InvokeRequest.h>

namespace Lambda {

	size_t invoke(const SubSystem *sub_system, const string &function_name, const string &payload_str) {

		Aws::Lambda::Model::InvokeRequest invoke_request;

		invoke_request.SetFunctionName(function_name);
		invoke_request.SetInvocationType(Aws::Lambda::Model::InvocationType::RequestResponse);
		invoke_request.SetLogType(Aws::Lambda::Model::LogType::Tail);

		std::shared_ptr<Aws::IOStream> payload = Aws::MakeShared<Aws::StringStream>("FunctionTest");

		*payload << payload_str;

		invoke_request.SetBody(payload);
		invoke_request.SetContentType("application/javascript");
		auto outcome = sub_system->lambda_client().Invoke(invoke_request);

		if (outcome.IsSuccess()) {
			auto &result = outcome.GetResult();

			// Lambda function result (key1 value)
			Aws::IOStream& payload = result.GetPayload();
			Aws::String function_result;
			std::getline(payload, function_result);
			std::cout << "Lambda result:\n" << function_result << "\n\n";
		}

		return 202;
	}

}
