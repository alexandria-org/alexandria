
import dot from './Dot.js'

class Getter {

	constructor() {
		this.memory = {
			getByIds: {},
			getSortedIds: {}
		}
		this.emptyObject = {}
		this.emptyArray = []
	}

	get(state, name) {
		return state._db[name] || {byId: this.emptyObject, allIds: this.emptyArray}
	}

	getById(state, name, id) {
		return this.get(state, name).byId[id] || this.emptyObject
	}

	/*
		Memoized function to retrieve multiple ids from database. Only returns ids that are there in the same order as
		the input.
	*/
	getByIds(state, name, ids) {
		if (ids === undefined) return this.emptyArray
		let db = this.get(state, name)
		let mem = this.memory.getByIds
		if (mem[name] !== undefined && mem[name][db.byId] !== undefined && mem[name][db.byId][ids] !== undefined) {
			return mem[name][db.byId][ids]
		}
		let objects = []
		for (let i in ids) {
			if (db.byId[ids[i]]) objects.push(db.byId[ids[i]])
		}
		if (mem[name] === undefined) mem[name] = {}
		if (mem[name][db.byId] === undefined) mem[name][db.byId] = {}
		mem[name][db.byId][ids] = objects
		return objects
	}

	getGlobal(state, name, defaultValue = undefined) {
		if (state._global[name] === undefined) return defaultValue
		return state._global[name]
	}

	// Set the specific normalized database data
	set(state, name, rows) {
		return dot.set(state, "_db." + name, rows)
	}

	// Append the normalized database data
	append(state, name, rows) {
		let allIds = dot.get(state, '_db.' + name + '.allIds', [])
		let byId = dot.get(state, '_db.' + name + '.byId', {})

		for (let i in rows.allIds) {
			let id = rows.allIds[i]
			allIds.push(id)
			byId[id] = rows.byId[id]
		}

		state = dot.set(state, '_db.' + name + '.allIds', allIds)
		state = dot.set(state, '_db.' + name + '.byId', byId)
		return state
	}

	// Memoized sort function.
	getSortedIds(state, name, orderBy, orderDirection) {
		let db = this.get(state, name)
		if (this.memory.getSortedIds[name] !== undefined) {
			let mem = this.memory.getSortedIds[name]
			if (mem.allIds === db.allIds && mem.orderBy === orderBy && mem.orderDirection === orderDirection) {
				return mem.sorted
			}
		}
		let ids = db.allIds.slice() // clone the ids
		let dir = orderDirection === "asc" ? -1 : 1
		ids.sort(function (id1, id2) {
			if (!orderBy) return 1
			return db.byId[id1][orderBy] < db.byId[id2][orderBy] ? dir : -dir
		})
		this.memory.getSortedIds[name] = {
			allIds: db.allIds,
			orderBy: orderBy,
			orderDirection: orderDirection,
			sorted: ids
		}
		return ids
	}

	getYear(date) {
		let parts = (date || '').split("-")
		if (parts.length === 3) return parts[0]
		return date || 'xxxx'
	}

	addDbItems(state, dbItems, name) {
		let allIds = dot.get(state, '_db.' + name + '.allIds', [])
		let byId = dot.get(state, '_db.' + name + '.byId', {})
		for (let i in dbItems.allIds) {
			let id = dbItems.allIds[i]
			if (!(id in byId)) {
				allIds.push(id)
			}
			byId[id] = dbItems.byId[id]
		}

		return dot.set(state, '_db.' + name, {
			byId: byId,
			allIds: allIds
		})
	}

	publicId(privateId) {
		return "id_" + privateId
	}

	privateId(publicId) {
		return publicId.replace("id_", "")
	}

}

export default new Getter()

