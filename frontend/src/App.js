import logo from './logo.svg';
import './App.css';
import React from 'react'
import {
	useLocation,
	Link,
	BrowserRouter
  } from "react-router-dom";

class App extends React.Component {

	constructor(props) {
		super(props)
	}
	
	render() {
		const {query} = this.props
		let url = new URLSearchParams(document.location.search)
		return (
			<BrowserRouter>
				<div className="search-box-front">
					<form method="get">
						<input type="text" name="q" value={query} />
						<button className="search-box-button" type="submit">Search</button>
					</form>
					<SearchResults name={url.get("q")} />
				</div>
			</BrowserRouter>
		)
	}

}

App.defaultProps = {
	query: ""
}

class SearchResults extends React.Component {
	render() {
		const {name} = this.props
		return (
			<div>
				{name}
			</div>
		  );
	}
}

export default App;
