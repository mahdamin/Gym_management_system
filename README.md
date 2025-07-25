
[README.md](https://github.com/user-attachments/files/21438110/README.md)
# Gym RFID Access Control System

A comprehensive gym access control system using ESP32 microcontroller and RC522 RFID reader with a modern web interface for user management and access monitoring.

## Features

- **RFID Access Control**: Secure access using MIFARE Classic, MIFARE Ultralight, and NTAG compatible cards
- **Web-based Management**: Modern, responsive web interface for user and system management
- **Real-time Monitoring**: Live dashboard with system status and recent activity
- **User Management**: Add, edit, and delete users with different access levels
- **Access Logging**: Comprehensive logging of all access attempts with timestamps
- **WiFi Configuration**: Easy WiFi setup through web interface
- **Multi-level Access**: Support for Member, Trainer, and Admin access levels

## Hardware Requirements

### Components
- ESP32 Development Board (ESP32-WROOM-32 or compatible)
- RC522 RFID Reader Module
- RFID Cards/Tags (MIFARE Classic 1K recommended)
- Jumper wires for connections
- Breadboard or PCB for prototyping
- 5V power supply (optional, can use USB power)

### Wiring Diagram

| ESP32 Pin | RC522 Pin | Function |
|-----------|-----------|----------|
| GPIO23 | MOSI | SPI Data Out |
| GPIO19 | MISO | SPI Data In |
| GPIO18 | SCK | SPI Clock |
| GPIO5 | SDA/SS | Chip Select |
| GPIO22 | RST | Reset |
| 3.3V | VCC | Power Supply |
| GND | GND | Ground |

## Software Requirements

- ESP-IDF v5.0 or later
- VS Code with ESP-IDF extension
- Python 3.7 or later
- Git

## Installation and Setup

### 1. Clone the Repository

```bash
git clone <repository-url>
cd gym_rfid_system
```

### 2. Install ESP-IDF

Follow the official ESP-IDF installation guide for your operating system:
- [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)

### 3. Set up the Project

```bash
# Set up ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Install dependencies
idf.py reconfigure
```

### 4. Configure the Project

```bash
# Open menuconfig for advanced configuration (optional)
idf.py menuconfig
```

### 5. Build the Project

```bash
# Build the firmware
idf.py build
```

### 6. Flash the Firmware

```bash
# Flash to ESP32 (replace /dev/ttyUSB0 with your port)
idf.py -p /dev/ttyUSB0 flash
```

### 7. Upload Web Files to SPIFFS

```bash
# Create SPIFFS image and flash
idf.py spiffs-flash
```

### 8. Monitor Serial Output

```bash
# Monitor serial output for debugging
idf.py -p /dev/ttyUSB0 monitor
```

## Configuration

### Initial Setup

1. **Power on the ESP32** - The system will start in AP mode if no WiFi credentials are stored
2. **Connect to WiFi AP** - Look for "GymRFID_Config" network (password: gym123456)
3. **Open web browser** - Navigate to http://192.168.4.1
4. **Configure WiFi** - Go to Settings tab and enter your WiFi credentials
5. **System will restart** - The ESP32 will connect to your WiFi network

### Default Credentials

- **Admin Username**: admin
- **Admin Password**: gym123456

**Important**: Change the default admin credentials after first setup for security.

### Adding Users

1. Open the web interface
2. Navigate to the "Users" tab
3. Click "Add User"
4. Fill in user details:
   - Name: User's full name
   - RFID UID: Scan a card or enter manually
   - Access Level: Member, Trainer, or Admin
5. Click "Save User"

## Web Interface

The system provides a comprehensive web interface accessible through any modern web browser:

### Dashboard
- Real-time system status
- Current card reader status
- Quick statistics
- Recent activity log

### User Management
- Add, edit, and delete users
- Assign access levels
- View user activity history

### Access Log
- Complete access history
- Filter and search capabilities
- Export functionality

### Settings
- WiFi configuration
- System information
- Admin controls

## API Documentation

The system provides a RESTful API for integration with external systems:

### Endpoints

#### System Status
```
GET /api/status
```
Returns system status including WiFi, RFID reader status, and uptime.

#### Card Data
```
GET /api/cards
```
Returns the last detected RFID card information.

#### User Management
```
GET /api/users          # Get all users
POST /api/users         # Create new user
PUT /api/users/{id}     # Update user
DELETE /api/users/{id}  # Delete user
```

#### Access Logs
```
GET /api/access-log     # Get recent access logs
```

#### Configuration
```
GET /api/config         # Get system configuration
POST /api/config        # Update configuration
```

## Troubleshooting

### Common Issues

#### ESP32 Not Connecting to WiFi
- Check WiFi credentials in settings
- Ensure WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Check signal strength and range

#### RFID Reader Not Working
- Verify wiring connections
- Check power supply (3.3V)
- Ensure RC522 module is not damaged
- Check SPI pin configuration

#### Web Interface Not Loading
- Check ESP32 IP address in serial monitor
- Ensure SPIFFS partition is flashed correctly
- Clear browser cache and cookies

#### Cards Not Being Detected
- Check card compatibility (MIFARE Classic recommended)
- Ensure proper distance (2-5cm from reader)
- Check for interference from other devices

### Debug Mode

Enable debug logging by modifying the log level in `sdkconfig`:

```
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
CONFIG_LOG_DEFAULT_LEVEL=4
```

## Security Considerations

### Network Security
- Change default admin credentials immediately
- Use strong WiFi passwords
- Consider using WPA3 if available
- Implement network segmentation for IoT devices

### Physical Security
- Secure the ESP32 device in a tamper-proof enclosure
- Protect wiring from physical access
- Consider backup power supply for critical installations

### Access Control
- Regularly review user access levels
- Remove inactive users promptly
- Monitor access logs for suspicious activity
- Implement time-based access restrictions if needed

## Development

### Project Structure

```
gym_rfid_system/
├── main/                   # Main application code
│   ├── main.c             # Application entry point
│   ├── wifi_manager.c/h   # WiFi management
│   ├── web_server.c/h     # HTTP server implementation
│   ├── rfid_manager.c/h   # RFID reader interface
│   ├── user_manager.c/h   # User management logic
│   └── storage_manager.c/h # Data persistence
├── data/                  # Web interface files
│   ├── index.html         # Main web page
│   ├── styles.css         # Styling
│   └── app.js            # JavaScript application
├── CMakeLists.txt         # Build configuration
├── partitions.csv         # Partition table
├── sdkconfig.defaults     # Default configuration
└── README.md             # This file
```

### Adding New Features

1. **Backend Changes**: Modify the appropriate manager module
2. **API Updates**: Add new endpoints in `web_server.c`
3. **Frontend Updates**: Update HTML, CSS, and JavaScript files
4. **Testing**: Test thoroughly with hardware setup

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Support

For support and questions:
- Check the troubleshooting section
- Review the ESP-IDF documentation
- Open an issue on the project repository

## Acknowledgments

- ESP-IDF framework by Espressif Systems
- RC522 library by abobija
- Modern web design principles and responsive layouts
- Open source community contributions

