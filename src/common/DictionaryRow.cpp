
#include "DictionaryRow.h"

DictionaryRow::DictionaryRow() {
}

DictionaryRow::DictionaryRow(const DictionaryRow &row) {
	m_columns = row.m_columns;
}

DictionaryRow::DictionaryRow(const string &row) {
	stringstream stream(row);
	read_stream(stream);
}

DictionaryRow::DictionaryRow(stringstream &stream) {
	read_stream(stream);
}

DictionaryRow::~DictionaryRow() {

}

int DictionaryRow::get_int(int column) const {
	return (int)m_columns[column];
}

float DictionaryRow::get_float(int column) const {
	return (float)m_columns[column];
}

double DictionaryRow::get_double(int column) const {
	return m_columns[column];
}

void DictionaryRow::read_stream(stringstream &stream) {
	string col;
	int i = 0;
	while (getline(stream, col, '\t')) {
		try {
			m_columns.push_back(stod(col));
		} catch(const invalid_argument &error) {

		} catch(const out_of_range &error) {
		}
		i++;
	}
}
