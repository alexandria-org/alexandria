
#include "TsvRow.h"

TsvRow::TsvRow(const string &line) {
	size_t pos_start = 0;
	size_t pos_end = 0;
	while (pos_end != string::npos) {
		pos_end = line.find(pos_start, '\t');
		m_cols.emplace_back(line.substr(pos_start, pos_end));
		pos_start = pos_end + 1;
	}
}

TsvRow::~TsvRow() {

}
