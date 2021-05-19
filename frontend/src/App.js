
import './App.css'
import React from 'react'
import * as ReactRedux from 'react-redux'
import {
	//Link,
	BrowserRouter
  } from "react-router-dom";
import TopMenu from './TopMenu.js'
import SearchResults from './SearchResults.js'
import SearchForm from './SearchForm.js'
import AboutPage from './AboutPage.js'

class App extends React.Component {
	
	componentDidMount() {
	}

	render() {
		const {page, query} = this.props
		return (
			<BrowserRouter>
				<div className="search-box-front">
					<div className="search-box-front-inner">
						<TopMenu />
						<h1><a href="/">Alexandria.org</a></h1>
						<SearchForm />
						{ page == "about" &&
							<AboutPage />
						}
						{ page == "" && query == "" &&
							<p>Search the web with alexandria.org</p>
						}
						{ page == "" && query != "" &&
							<SearchResults />
						}
					</div>
				</div>
			</BrowserRouter>
		)
	}

}

/* Connected Props */
export default ReactRedux.connect(function (state, props) {
	return {
		query: state.query || "",
		page: state.page || ""
	}
})(App)

