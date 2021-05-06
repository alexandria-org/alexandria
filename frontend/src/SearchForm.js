
import React from 'react'
import * as ReactRedux from 'react-redux'
import Getter from './Getter.js'
import dot from './Dot.js'
import actions from './Actions.js'

const component_name = 'SearchForm'

class SearchForm extends React.Component {

	render() {
		const {query} = this.props
		return (
			<form className="search-form" method="get">
				<input type="text" name="q" value={query} onChange={this.handle_change} />
				<button className="search-box-button" type="submit">Search</button>
			</form>
		)
	}

	handle_change(event) {
		actions.dispatch(actions.action('SET_QUERY', {
			value: event.target.value
		}, component_name))
	}
}

SearchForm.defaultProps = {
	query: ""
}

/* Actions */
actions.addComponentReducer(component_name, function (state, action) {
	switch (action.type) {
		case 'SET_QUERY':
			return dot.set(state, 'query', action.value)
	}

	return state
})

/* Connected Props */
export default ReactRedux.connect(function (state, props) {
	return {
		query: state.query || ''
	}
})(SearchForm)
