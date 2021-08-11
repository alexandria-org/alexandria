
#pragma once

#include <iostream>
#include "system/SubSystem.h"

using namespace std;

namespace Lambda {

	size_t invoke(const SubSystem *sub_system, const string &function_name, const string &payload);

}
