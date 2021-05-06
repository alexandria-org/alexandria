
import './App.css'
import React from 'react'
import {
	//Link,
	BrowserRouter
  } from "react-router-dom";
import SearchResults from './SearchResults.js'
import SearchForm from './SearchForm.js'
class App extends React.Component {
	
	componentDidMount() {
	}

	render() {
		return (
			<BrowserRouter>
				<div className="search-box-front">
					<SearchForm />
					<SearchResults />
				</div>
			</BrowserRouter>
		)
	}

}


export default App
