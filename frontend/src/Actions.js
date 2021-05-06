
import {createStore, applyMiddleware} from 'redux'
import thunk from 'redux-thunk'
import dot from './Dot.js'

var logger;
if (process.env.NODE_ENV === 'development') {
	logger = require('redux-logger');
}

class Actions {

	constructor(initialState) {

		initialState._global = {_openModal: ''}
		if (initialState._db === undefined) {
			initialState._db = {}
		}
		this.reducer = this.reducer.bind(this)
		this.reducers = {}
		this.componentReducers = {}

		if (logger === undefined) {
			this.store = createStore(this.reducer, initialState, applyMiddleware(thunk))
		} else {
			//this.store = createStore(this.reducer, initialState, applyMiddleware(thunk))
			this.store = createStore(this.reducer, initialState, applyMiddleware(thunk, logger.default))
		}
	}

	action(type, parameters = {}, reducer = null) {
		let action = {
			type: type,
			...parameters
		};
		if (reducer) {
			action.reducer = reducer
		}
		return action
	}

	dispatch(action) {

		return this.store.dispatch(action);
	}

	getState() {
		return this.store.getState();
	}

	reducer(state, action) {

		if (action.reducer !== undefined && this.componentReducers[action.reducer] !== undefined) {
			return this.componentReducers[action.reducer](state, action)
		}

		if (action.type === "DUMMY_ACTION") {
			state._global = dot.set(state._global, "DUMMY", "ACTION")
			return state
		}

		// Run global actions
		if (action.type === "SET_INITIAL_STATE") {
			state = this.reducers[action.reducerName].initialState(state);
		}

		if (action.type === "ADD_INITIAL_STATE") {
			if (action.stateScope === "") {
				for (let i in action.initialState) {
					if (i === "_db") {
						for (let dbName in action.initialState[i]) {
							state = dot.combine(state, '_db.' + dbName + '.byId', action.initialState._db[dbName].byId)
							state = dot.combine(state, '_db.' + dbName + '.allIds', action.initialState._db[dbName].allIds)
						}
					} else {
						state = dot.set(state, i, action.initialState[i]);
					}
				}
			} else {
				state = dot.set(state, action.stateScope, dot.get(action.initialState, action.stateScope));
			}
		}

		if (action.type === "OPEN_MODAL") {
			state = dot.set(state, '_global._openModal', action.modalName)
		}

		if (action.type === "CLOSE_MODAL") {
			state = dot.set(state, '_global._openModal', '')
		}

		for (let applicationName in this.reducers) {
			state = this.reducers[applicationName].reducer(state, action);
		}

		return state;
	}

	addReducer(name, reducer) {
		this.reducers[name] = reducer;

		if (this.reducers[name].initialState !== undefined) {
			this.dispatch(this.action('SET_INITIAL_STATE', {reducerName: name}));
		}
	}

	addComponentReducer(name, reducer) {
		this.componentReducers[name] = reducer;
	}

	defaultState(state, path, defaultValues) {
		if (dot.get(state, path)) return state;
		return dot.set(state, path, defaultValues);
	}

	addInitialState(stateScope, initialState) {
		this.dispatch(this.action('ADD_INITIAL_STATE', {stateScope: stateScope, initialState: initialState}));
	}

	quick(name, parameters) {
		return (e) => {
			e.preventDefault();
			this.dispatch(this.action(name, parameters))
		}
	}

	httpEvent(url, method, body, initiateEvent, successEvent, failureEvent) {
		return function (dispatch) {

			dispatch(initiateEvent())

			let request = {
				credentials: 'same-origin',
				method: method
			}

			if (method !== "GET") {
				request.body = JSON.stringify(body)
			}

			return fetch(url, request)
			.then(function (response) {
				response.json().then(function (response) {
					if (response.status === 'success') {
						dispatch(successEvent(response.message))
					} else {
						dispatch(failureEvent(response.message.reason))
					}
				}).catch(function (error) {
					dispatch(failureEvent(error.message))
				})
			})
			.catch(function (error) {
				dispatch(failureEvent(error.message))
			})
		}
	}

}

let url = new URLSearchParams(document.location.search)

export default new Actions({query: url.get("q")})
