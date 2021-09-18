/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
