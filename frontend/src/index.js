import React from 'react';
import ReactDOM from 'react-dom';
import './index.css';
import App from './App';
import actions from './Actions.js'
import reportWebVitals from './reportWebVitals';
import * as ReactRedux from 'react-redux';

ReactDOM.render(
	<ReactRedux.Provider store={actions.store}>
		<React.StrictMode>
			<App />
		</React.StrictMode>
	</ReactRedux.Provider>
,
  document.getElementById('root')
);

// If you want to start measuring performance in your app, pass a function
// to log results (for example: reportWebVitals(console.log))
// or send to an analytics endpoint. Learn more: https://bit.ly/CRA-vitals
reportWebVitals();
