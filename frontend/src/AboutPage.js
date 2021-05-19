
import React from 'react'
import * as ReactRedux from 'react-redux'
import Getter from './Getter.js'
import dot from './Dot.js'
import actions from './Actions.js'

const component_name = 'AboutPage'

class AboutPage extends React.Component {

	componentDidMount() {

	}

	render() {
		const {} = this.props
		return (
			<div className="sub-page">
				<p>Alexandria.org is a general search engine at an early stage of development.</p>
				<p>You can email us at info at alexandria dot org if you have questions, suggestions or want to get involved. </p>
			</div>
		  );
	}
}

AboutPage.defaultProps = {
}

/* Actions */
actions.addComponentReducer(component_name, function (state, action) {
	switch (action.type) {
		case 'LOAD_INITIATED':
			state = dot.set(state, '_loading_error', "")
			return dot.set(state, '_loading_results', true)
	}

	return state
})

/* Connected Props */
export default ReactRedux.connect(function (state, props) {
	return {
	}
})(AboutPage)
