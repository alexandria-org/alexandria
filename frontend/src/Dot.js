
class Dot {

	set(state, path, value) {
		let parts = path.split('.')
		return this._set(state, parts, value)
	}

	_set(state, paths, value) {
		let ret
		if (Array.isArray(state)) {
			ret = Object.assign([], state)
		} else {
			ret = Object.assign({}, state)
		}

		if (paths.length === 1) {
			let path = paths.shift()
			if (Array.isArray(ret) && !isNaN(parseInt(path))) {
				path = parseInt(path)
			}
			ret[path] = value
		} else {
			let path = paths.shift()
			if (Array.isArray(ret) && !isNaN(parseInt(path))) {
				path = parseInt(path)
			}
			ret[path] = this._set(ret[path], paths, value)
		}
		return ret
	}

	/*
		Sets the value parameter on path but instead of removing any existing properties on path we just add the
		new ones, overwriting the value. Ex:
		combine({path: {test1: 'test1', test2: 'test2'}}, 'path', {test2: 'new value'})

		will return
		{path: {test1: 'test1', test2: 'new value'}}
	*/
	combine(state, path, value) {
		let oldValue = this.get(state, path, null)
		if (Array.isArray(oldValue) && Array.isArray(value)) {
			return this.set(state, path, oldValue.concat(value))
		} else if (typeof oldValue === 'object' && oldValue !== null && typeof value === 'object' && value !== null) {
			let newObject = {}
			for (let i in oldValue) {
				newObject[i] = oldValue[i]
			}
			for (let i in value) {
				newObject[i] = value[i]
			}
			return this.set(state, path, newObject)
		} else {
			return this.set(state, path, value)
		}
	}

	remove(state, path) {
		let parts = path.split('.')
		return this._remove(state, parts)
	}

	_remove(state, paths) {
		let ret = Object.assign({}, state)
		if (paths.length === 1) {
			// Do nothing...
			delete ret[paths[0]]
		} else {
			let path = paths.shift()
			ret[path] = this._set(ret[path], paths)
		}
		return ret
	}

	get(state, path, defaultValue = '') {
		let parts = path.split('.')
		let obj = state
		for (let i in parts) {
			let name = parts[i]

			if (Array.isArray(obj)) {
				let ind = parseInt(name)
				if (Number.isInteger(ind)) {
					if (ind < 0 || ind >= obj.length) return defaultValue
					obj = obj[ind]
				}
			} else if (typeof obj === 'object' && obj !== null) {
				if (obj[name] === undefined) return defaultValue
				obj = obj[name]
			} else {
				return defaultValue
			}
		}
		return obj
	}
	
}

if (false) {

	var dot = new Dot()

	var state = {
		person1: {
			name: "Erica",
			state: "Hungrig"
		},
		person2: {
			name: "Josef",
			state: "Programmeringsnördig"
		},
		numPersons: 2,
		personNames: ["Erica", "Josef"]
	}

	var test = true
	test = test && dot.get(state, "person1.name.test", "Unnamed") === "Unnamed"
	test = test && dot.get(state, "person1.name", "Unnamed") === "Erica"
	test = test && dot.get(state, "person2", "Unnamed") === state.person2
	test = test && dot.get(state, "numPersons", "Unnamed") === 2
	test = test && dot.get(state, "personNames.0", "Unnamed") === "Erica"
	test = test && dot.get(state, "personNames.1", "Unnamed") === "Josef"
	test = test && dot.get(state, "personNames.2", "Unnamed") === "Unnamed"

	let oldState = state
	state = dot.set(state, "person3.name", "Ivan")
	state = dot.set(state, "person3.state", "Sugen på att spela aoe")

	test = test && dot.get(state, "person3.name") === "Ivan"
	test = test && dot.get(state, "person3.state") === "Sugen på att spela aoe"

	// Check immutability.
	test = test && (oldState !== state)
	test = test && oldState.person1 === state.person1
	test = test && oldState.person2 === state.person2

	state = dot.set(state, "person3.state", "Dålig förlorare")

	// Check immutability.
	test = test && oldState !== state
	test = test && oldState.person1 === state.person1
	test = test && oldState.person2 === state.person2
	test = test && oldState.person3 !== state.person3

	// Test remove
	state = dot.remove(state, "person1.name")
	test = test && oldState !== state
	test = test && oldState.person1 !== state.person1
	test = test && oldState.person2 === state.person2
	test = test && oldState.person3 !== state.person3
	test = test && state.person1.name === undefined
	test = test && state.person1.state === "Hungrig"

	// Test get default values
	state = {}
	let empty = []
	test = test && dot.get(state, "_db.asd.asd2", "test123") === "test123"
	test = test && dot.get(state, "_db.asd.asd2", empty) === empty
	let state2 = {db: {asd: {}}}
	test = test && dot.get(state2, "_db.asd.asd2", "test123") === "test123"
	test = test && dot.get(state2, "_db.asd.asd2", empty) === empty

	let state3 = {_db: {works: {byId: {new_work_elVXFKcwAd0C: {}}}}}
	test = test && dot.get(state3, '_db.works.byId.new_work_elVXFKcwAd0C.workTitleIds', empty) === empty

	let state4 = {test: {results: [{nr1: false}, {nr2: false}, {nr3: false}]}};
	state4 = dot.set(state4, "test.results.0.nr1", true)
	test = test && Array.isArray(state4.test.results)

	let state5 = {
		editReviewModal: {test: 'asd'},
		editReviewModal2: {test2: 'asd2'}
	}
	state5 = dot.remove(state5, 'editReviewModal')
	test = test && state5.editReviewModal === undefined && state5.editReviewModal2 !== undefined

	state5 = {
		editReviewModal: {test: 'asd'},
		editReviewModal2: {test2: 'asd2'}
	}
	state5 = dot.remove(state5, 'editReviewModal.test')
	test = test && state5.editReviewModal !== undefined && state5.editReviewModal2 !== undefined &&
		state5.editReviewModal.test === undefined


	let state6 = {
		editReviewModal: {test: 'asd', deep: {test3: 'asd3'}}
	}
	state6 = dot.combine(state6, 'editReviewModal', {
		test2: 'asd2',
		deep: {test4: 'asd4'}
	})

	test = test && state6.editReviewModal.test === 'asd' && state6.editReviewModal.test2 === 'asd2'
	test = test && state6.editReviewModal.deep.test3 === undefined && state6.editReviewModal.deep.test4 === 'asd4'

	let test7 = dot.combine({path: {test1: 'test1', test2: 'test2'}}, 'path', {test2: 'new value'})
	test = test && test7.path.test1 === 'test1' && test7.path.test2 === 'new value'

	console.log("Dot tests: " + (test ? 'passed' : 'failed'))
}

export default new Dot()

