
import "react-loader-spinner/dist/loader/css/react-spinner-loader.css";
import Loader from "react-loader-spinner";
import './SearchResult.css'
import React from 'react'
import * as ReactRedux from 'react-redux'
import Getter from './Getter.js'
import dot from './Dot.js'
import actions from './Actions.js'

const component_name = 'SearchInfo'

class SearchInfo extends React.Component {

	componentDidMount() {
	}

	render() {
		const {file_size, file_unzipped_size, download_time, parse_time, sort_time, num_lines} = this.props
		return (
			<div className="search-info">
				file_size: {Math.round(file_size/(1024*1024)*100)/100}mb<br/>
				file_unzipped_size: {Math.round(file_unzipped_size/(1024*1024)*100)/100}mb<br/>
				download_time: {download_time}ms<br/>
				parse_time: {parse_time}ms<br/>
				sort_time: {sort_time}ms<br/>
				num_lines: {num_lines}
			</div>
		  );
	}

}

SearchInfo.defaultProps = {
	file_size: 0,
	file_unzipped_size: 0,
	download_time: 0,
	parse_time: 0,
	sort_time: 0,
	num_lines: 0
}

/* Actions */
actions.addComponentReducer(component_name, function (state, action) {
	switch (action.type) {
		case 'DUMMY':
			state = dot.set(state, '_loading_error', "")
			return dot.set(state, '_loading_results', true)
	}

	return state
})

/* Connected Props */
export default ReactRedux.connect(function (state, props) {
	let search_info = state._search_info || {}
	return {
		...search_info
	}
})(SearchInfo)
