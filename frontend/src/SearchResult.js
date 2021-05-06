
import './SearchResult.css'
import React from 'react'
import * as ReactRedux from 'react-redux'
import Getter from './Getter.js'
import dot from './Dot.js'
import actions from './Actions.js'

class SearchResult extends React.Component {
	render() {
		const {url, title, snippet} = this.props
		return (
			<div className="search-result">
				<div>
					<a href={url}>
						<cite>{url}</cite><br/>					
						<h3>{title}</h3>
					</a>
					<p>{snippet}</p>
				</div>
			</div>
		  );
	}

}

SearchResult.defaultProps = {
	search_result_ids: [],
	query: ""
}

/* Connected Props */
export default ReactRedux.connect(function (state, props) {
	let search_result = Getter.getById(state, "search_results", props.id)
	return {
		...search_result
	}
})(SearchResult)
