
template <typename DataRecord>
struct SearchStorage {

	vector<FullTextResultSet<DataRecord> *> result_sets;
	FullTextResultSet<DataRecord> *intersected_result;

};
