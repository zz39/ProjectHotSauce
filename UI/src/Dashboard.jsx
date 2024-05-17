import React from 'react';
import { Link } from 'react-router-dom';

const Dashboard = () => {
  return (
    <div className="dashboard-container">
      {/* Navbar */}
      <nav className="navbar">
        <div className="navbar-brand">
          <Link to="/" className="navbar-item">
            Your Logo
          </Link>
        </div>
        {/* Add any additional navbar items or components here */}
      </nav>

      {/* Sidebar */}
      <aside className="sidebar">
        <ul>
          <li><Link to="/sensors">Sensors</Link></li>
          <li><Link to="/alerts">Alerts</Link></li>
          <li><Link to="/download">Download Data</Link></li>
          {/* Add more sidebar links as needed */}
        </ul>
      </aside>

      {/* Main Content */}
      <main className="main-content">
        {/* Your dashboard content goes here */}
        <h1>Welcome to the Dashboard!</h1>
        {/* Add more dashboard components and content here */}
      </main>
    </div>
  );
};

export default Dashboard;
