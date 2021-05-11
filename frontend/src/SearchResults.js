
import "react-loader-spinner/dist/loader/css/react-spinner-loader.css";
import Loader from "react-loader-spinner";
import './SearchResult.css'
import React from 'react'
import * as ReactRedux from 'react-redux'
import Getter from './Getter.js'
import dot from './Dot.js'
import actions from './Actions.js'
import SearchResult from './SearchResult.js'
import SearchInfo from './SearchInfo.js'

const component_name = 'SearchResults'

class SearchResults extends React.Component {

	componentDidMount() {
		console.log(this.props)
		if (this.props.query.trim().length > 0) {
			this.load_search_results()
		}
	}

	render() {
		const {query, search_result_ids, loading, error} = this.props
		return (
			<div className="search-results">
				{ loading &&
					<Loader
						type="Puff"
						color="#00BFFF"
						height={100}
						width={100}
					/>
				}
				{ error != '' &&
					<div className="error">{error}</div>
				}
				<SearchInfo />
				{this.render_search_results()}
			</div>
		  );
	}

	render_search_results() {
		let elements = []
		const {search_result_ids} = this.props
		for (let id in search_result_ids) {
			elements.push(<SearchResult id={id} key={id} />)
		}

		return elements
	}

	load_search_results() {
		actions.dispatch(actionLoad(this.props.query))
	}
}

SearchResults.defaultProps = {
	search_result_ids: [],
	query: "",
	loading: false,
	error: ''
}

/* Actions */
actions.addComponentReducer(component_name, function (state, action) {
	switch (action.type) {
		case 'LOAD_INITIATED':
			state = dot.set(state, '_loading_error', "")
			return dot.set(state, '_loading_results', true)
		case 'LOAD_SUCCESS':
			state = dot.set(state, '_search_info', action.response.debug)
			let db_obj = {
				allIds: [],
				byId: {}
			}
			for (let i in action.response.results) {
				db_obj.allIds.push(i)
				db_obj.byId[i] = action.response.results[i]
			}
			state = dot.set(state, '_loading_results', false)
			return dot.set(state, '_db.search_results', db_obj)
		case 'LOAD_FAILURE':
			state = dot.set(state, '_loading_error', action.reason)
			return dot.set(state, '_loading_results', false)
	}

	return state
})

function actionLoad(query) {
	return actions.httpEvent(
		// Url
		'https://7aiplhf4n4.execute-api.us-east-1.amazonaws.com/live?query=' + query,

		// Method
		"GET",

		// Body
		{
		},

		// Initiate event
		() => actions.action('LOAD_INITIATED', {}, component_name),

		// Success event
		(response) => actions.action('LOAD_SUCCESS', {
			response: response
		}, component_name),

		// Failure event
		(reason) => actions.action('LOAD_FAILURE', {
			reason: reason
		}, component_name)
	)
}

/* Connected Props */
export default ReactRedux.connect(function (state, props) {
	return {
		query: state.query || "",
		loading: state._loading_results || false,
		error: state._loading_error || '',
		search_result_ids: Getter.get(state, "search_results").allIds
	}
})(SearchResults)
