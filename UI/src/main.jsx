import React from 'react'
import ReactDOM from 'react-dom/client'

import Dashboard from './Dashboard.jsx';
import App from './App.jsx'

import {
  createBrowserRouter,
  RouterProvider,
  Route,
} from "react-router-dom";

const router = createBrowserRouter([
  {
    path: '/app',
    element: <App />
  },
  {
    path: "/",
    element: <Dashboard />,
  }
]);

ReactDOM.createRoot(document.getElementById('root')).render(
  <React.StrictMode>
    <RouterProvider router={router} />
  </React.StrictMode>,
)