# Contributing to ESP32 Mesh Communicator

Thank you for your interest in contributing to the ESP32 Mesh Communicator project! This document provides guidelines and information for contributors.

## 🤝 How to Contribute

### Reporting Issues
- Use GitHub Issues to report bugs or request features
- Search existing issues before creating new ones
- Provide detailed information including:
  - Hardware configuration
  - Software versions
  - Steps to reproduce
  - Expected vs actual behavior
  - Serial monitor output (if applicable)

### Submitting Changes
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Test thoroughly on actual hardware
5. Commit with clear messages (`git commit -m 'Add amazing feature'`)
6. Push to your branch (`git push origin feature/amazing-feature`)
7. Open a Pull Request

## 📋 Development Guidelines

### Code Style
- Follow Arduino coding conventions
- Use meaningful variable and function names
- Comment complex logic and hardware-specific code
- Keep functions focused and reasonably sized
- Use consistent indentation (2 spaces)

### Hardware Testing
- Test all changes on actual ESP32 hardware
- Verify functionality with all connected modules
- Test edge cases (no GPS signal, WiFi disconnection, etc.)
- Document any hardware-specific requirements

### Documentation
- Update README.md for new features
- Add wiring diagrams for hardware changes
- Update library requirements if needed
- Include usage examples for new functionality

## 🔧 Development Setup

### Prerequisites
- Arduino IDE 1.8.19 or later
- ESP32 board package installed
- All required libraries (see LIBRARIES.md)
- ESP32 development board with connected modules

### Local Development
1. Clone your fork: `git clone https://github.com/yourusername/esp32-mesh-communicator.git`
2. Open `esp32_mesh_communicator.ino` in Arduino IDE
3. Install required libraries
4. Select "ESP32 Dev Module" board
5. Connect hardware and test

### Testing Checklist
- [ ] Code compiles without warnings
- [ ] All modules initialize correctly
- [ ] Basic functionality works (send/receive messages)
- [ ] GPS functionality tested outdoors
- [ ] WiFi web interface accessible
- [ ] SD card logging works
- [ ] SOS emergency mode functions
- [ ] No memory leaks or crashes during extended use

## 🐛 Bug Reports

### Before Reporting
- Update to latest version
- Check existing issues
- Test with minimal hardware setup
- Verify wiring connections

### Bug Report Template
```markdown
**Hardware Configuration:**
- ESP32 board model:
- LoRa module:
- GPS module:
- Display type:
- Power supply:

**Software Environment:**
- Arduino IDE version:
- ESP32 core version:
- Library versions:

**Issue Description:**
Clear description of the problem

**Steps to Reproduce:**
1. Step one
2. Step two
3. Step three

**Expected Behavior:**
What should happen

**Actual Behavior:**
What actually happens

**Serial Output:**
```
Paste serial monitor output here
```

**Additional Context:**
Any other relevant information
```

## ✨ Feature Requests

### Feature Request Template
```markdown
**Feature Description:**
Clear description of the proposed feature

**Use Case:**
Why is this feature needed?

**Proposed Implementation:**
How should this feature work?

**Hardware Requirements:**
Any additional hardware needed?

**Alternatives Considered:**
Other ways to achieve the same goal
```

## 🔀 Pull Request Guidelines

### PR Checklist
- [ ] Code follows project style guidelines
- [ ] Changes are tested on hardware
- [ ] Documentation is updated
- [ ] Commit messages are clear
- [ ] PR description explains the changes
- [ ] No merge conflicts

### PR Template
```markdown
**Description:**
Brief description of changes

**Type of Change:**
- [ ] Bug fix
- [ ] New feature
- [ ] Documentation update
- [ ] Performance improvement
- [ ] Code refactoring

**Testing:**
- [ ] Tested on ESP32 hardware
- [ ] All modules function correctly
- [ ] No regression in existing features

**Screenshots/Videos:**
If applicable, add visual evidence

**Additional Notes:**
Any other relevant information
```

## 📚 Areas for Contribution

### High Priority
- Mesh routing implementation
- Power management and sleep modes
- Message encryption
- Performance optimizations
- Bug fixes and stability improvements

### Medium Priority
- Additional display support
- Alternative GPS modules
- Mobile app companion
- Weather station integration
- Voice message recording

### Low Priority
- Bluetooth connectivity
- Solar charging support
- Custom enclosure designs
- Additional language support

## 🏗️ Architecture Overview

### Core Components
- **State Machine**: Handles UI navigation and device states
- **LoRa Communication**: Message transmission and reception
- **GPS Integration**: Location tracking and sharing
- **Display System**: OLED interface and menu system
- **Storage Management**: SD card and EEPROM operations
- **Web Interface**: HTTP server for remote control

### Key Files
- `esp32_mesh_communicator.ino`: Main sketch file
- `README.md`: Project documentation
- `docs/WIRING_GUIDE.md`: Hardware assembly guide
- `docs/LIBRARIES.md`: Dependencies documentation

## 🧪 Testing Framework

### Unit Testing
Currently, testing is primarily done on hardware. Consider contributing:
- Mock hardware interfaces for unit testing
- Automated testing scripts
- Continuous integration setup

### Integration Testing
- Test complete message flow between devices
- Verify GPS accuracy and timing
- Test web interface functionality
- Validate emergency SOS features

## 📖 Documentation Standards

### Code Comments
```cpp
/**
 * Brief function description
 * @param parameter Description of parameter
 * @return Description of return value
 */
bool functionName(int parameter) {
    // Implementation details
}
```

### Markdown Documentation
- Use clear headings and structure
- Include code examples where helpful
- Add diagrams for complex concepts
- Keep language clear and concise

## 🚀 Release Process

### Version Numbering
- Follow Semantic Versioning (MAJOR.MINOR.PATCH)
- MAJOR: Breaking changes
- MINOR: New features, backward compatible
- PATCH: Bug fixes, backward compatible

### Release Checklist
- [ ] All tests pass
- [ ] Documentation updated
- [ ] Version number incremented
- [ ] Release notes prepared
- [ ] Tagged in Git

## 💬 Community Guidelines

### Code of Conduct
- Be respectful and inclusive
- Focus on constructive feedback
- Help newcomers learn
- Acknowledge contributions
- Keep discussions on-topic

### Communication Channels
- GitHub Issues: Bug reports and feature requests
- GitHub Discussions: General questions and ideas
- Pull Request comments: Code-specific discussions

## 🎯 Getting Started

### First Contribution Ideas
- Fix typos in documentation
- Add code comments
- Test on different hardware configurations
- Improve error messages
- Add usage examples

### Mentorship
- Experienced contributors welcome questions
- Tag issues with "good first issue" for newcomers
- Provide detailed feedback on pull requests
- Share knowledge about ESP32 development

## 📞 Contact

- **Project Maintainer**: [GitHub Issues](https://github.com/shafimahadi/esp32-mesh-communicator/issues)
- **General Questions**: [GitHub Discussions](https://github.com/shafimahadi/esp32-mesh-communicator/discussions)
- **Security Issues**: Create a private issue or contact maintainers directly

---

**Thank you for contributing to the ESP32 Mesh Communicator project! Your contributions help make reliable offline communication accessible to everyone.**
