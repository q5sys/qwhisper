# QWhisper

QWhisper is a Qt6-based desktop application that provides real-time speech-to-text functionality using OpenAI's Whisper model. It offers a user-friendly graphical interface for continuous audio transcription with support for multiple output formats and advanced audio processing features.

![qwhisper](images/qwhisper.png)

## Features

- Real-time speech-to-text transcription using Whisper models
- Support for multiple Whisper model sizes (tiny, base, small, medium, large, turbo)
- CPU and GPU (CUDA) acceleration support
- Voice Activity Detection with configurable thresholds
- Audio filtering with bandpass filter for improved accuracy
- Multiple audio input sources (microphone, system audio)
- Interactive transcript editing with search functionality
- Multiple output options:
  - Interactive transcript window
  - File output with continuous append
  - Clipboard integration
  - Direct typing to active window (X11/Wayland)
- Automatic model downloading and management
- Persistent configuration settings
- Audio level monitoring and visualization

## System Requirements

### Required Dependencies

- Qt6 (Core, Widgets, Multimedia, Network)
- CMake 3.16 or higher
- C++17 compatible compiler
- PulseAudio development libraries (Linux)

### Optional Dependencies

- CUDA Toolkit (for GPU acceleration)
- X11 development libraries with XTest extension (for window typing on X11)
- For Wayland window typing: 
  - (may require xwayland)
  - `ydotool` (requires ydotoold daemon) or
  - `wtype` (alternative to ydotool)

## Building from Source

### Prerequisites

- c++/qt build toolchain

### Build Steps

1. Clone or extract the source code
2. Navigate to the qwhisper directory
3. Create and enter build directory:
   ```bash
   mkdir -p build
   cd build
   ```
4. Configure the build:
   ```bash
   cmake ..
   ```
5. Compile the application:
   ```bash
   make -j$(nproc)
   ```

The build process will automatically download and compile the whisper.cpp library. CUDA support will be automatically detected and enabled if the CUDA toolkit is installed.

## Installation

### System Installation

To install QWhisper system-wide:
```bash
sudo ./install.sh
```

### User Installation

To install for the current user only:
```bash
./install.sh
```

The installer will:
- Copy the executable to the appropriate bin directory
- Install the application icon and desktop integration files
- Set up documentation and menu entries

### Running the Application

After installation, QWhisper can be launched:
- From the command line: `qwhisper`
- From your desktop environment's application menu
- By clicking the desktop icon

To run without installation from the build directory:
```bash
cd build
./qwhisper
```

## Configuration

QWhisper stores its configuration in `~/.config/qwhisper/config.json` following the XDG Base Directory specification. Whisper models are downloaded to `~/.local/share/qwhisper/models/` by default.

### First Run

On first launch, QWhisper will prompt to download a Whisper model if none are found. The application supports automatic downloading of all official Whisper models from the Hugging Face repository.

### Keyboard Shortcuts

- `Ctrl+F`: Search within transcript
- `Ctrl+S`: Save transcript
- `Ctrl+O`: Open saved transcript
- `Ctrl+E`: Export transcript

## Uninstallation

To remove QWhisper from your system:
```bash
./uninstall.sh
```

The uninstaller will:
- Remove all installed application files
- Optionally preserve user configuration files
- Optionally preserve downloaded models
- Clean up desktop integration

## Troubleshooting

### CUDA Issues

If you experience crashes with GPU selection, try:
```bash
CUDA_VISIBLE_DEVICES="0,1,2" qwhisper
```
This limits which GPUs are visible to avoid out-of-memory issues.


### Audio Issues

If audio capture is not working:
- Check that your audio device is properly configured in system settings
- Verify PulseAudio is running and accessible
- Try different audio input sources in QWhisper settings

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
