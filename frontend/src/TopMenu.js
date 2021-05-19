
import React from 'react'
import * as ReactRedux from 'react-redux'
import Getter from './Getter.js'
import dot from './Dot.js'
import actions from './Actions.js'

const component_name = 'TopMenu'

class TopMenu extends React.Component {

	componentDidMount() {

	}

	render() {
		const {} = this.props
		return (
			<div className="top-menu">
				<a href="/?p=about">About</a>
			</div>
		  );
	}
}

TopMenu.defaultProps = {
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
})(TopMenu)
