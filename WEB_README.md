# Web Interface Setup and Usage Guide

## Overview
This web interface provides real-time visualization of the Multikernel OS, perfect for live demonstrations and presentations.

## Prerequisites
- Python 3.8+
- Built multikernel_os executable

## Installation

1. Install Python dependencies:
```bash
pip3 install -r requirements.txt
```

2. Build the multikernel OS:
```bash
cmake .
make
```

## Running the Web Interface

1. Start the web server:
```bash
python3 web_server.py
```

2. Open your browser and navigate to:
```
http://localhost:5000
```

## Features

### Real-Time Dashboard
- **Per-Core Monitoring**: Visual cards showing load for each of the 8 cores
- **Live Statistics**: 
  - Total processes created
  - Total messages exchanged
  - Active cores count
  - Average core load
- **System Logs**: Real-time log streaming from the OS
- **Interactive Controls**: Start/Stop system, refresh statistics

### WebSocket Support
The interface uses WebSockets for real-time updates without page refresh.

## For Presentations

1. Start the web server before your presentation
2. Open the dashboard in full-screen mode
3. Click "Start System" to begin the demonstration
4. The dashboard will automatically update with:
   - Process creations
   - Core load changes
   - Message passing events
   - System statistics

## Troubleshooting

**Port already in use:**
```bash
# Kill the process using port 5000
lsof -ti:5000 | xargs kill -9
```

**Flask not found:**
```bash
pip3 install --user -r requirements.txt
```

**Executable not found:**
Make sure you've built the project:
```bash
cmake . && make
```

## Architecture

```
┌─────────────────────────────────────────┐
│     Web Browser (JavaScript)            │
│  - Dashboard UI                         │
│  - WebSocket Client                     │
└───────────────┬─────────────────────────┘
                │ HTTP/WebSocket
┌───────────────▼─────────────────────────┐
│     Flask Web Server (Python)           │
│  - REST API                             │
│  - WebSocket Handler                    │
│  - Process Manager                      │
└───────────────┬─────────────────────────┘
                │ subprocess
┌───────────────▼─────────────────────────┐
│     multikernel_os (C++)                │
│  - 8 Core Kernels                       │
│  - Message Passing                      │
│  - Process Management                   │
└─────────────────────────────────────────┘
```
