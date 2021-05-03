
#pragma once

#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>

#define CC_ERROR 1
#define CC_OK 0

Aws::Client::ClientConfiguration get_s3_config();
Aws::S3::S3Client get_s3_client();
