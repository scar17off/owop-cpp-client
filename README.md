# OWOP Client

A modern C++ client implementation for Our World of Pixels (OWOP), featuring a clean OpenGL-based renderer and ImGui interface.

## Features

- OpenGL-based chunk rendering with MSAA support
- ImGui-powered user interface
- Real-time collaborative pixel art
- Zoom and pan controls
- Color picker with RGB input
- Multiple tool support (Cursor, Move, Pipette)
- WebSocket-based networking
- Cross-platform compatibility

## Dependencies

- GLFW 3.x
- OpenGL 3.3+
- Dear ImGui
- GLAD
- A modern C++ compiler supporting C++17

## Building

### Prerequisites

1. CMake 3.15 or higher
2. A C++17 compatible compiler
3. vcpkg (recommended) or manual installation of dependencies

### Build Steps 

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Usage

1. Launch the application
2. Enter server details in the Settings window
3. Click "Connect" to join a world
4. Use mouse wheel to zoom in/out
5. Drag with left mouse button to pan
6. Select tools and colors from the Tools window

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.