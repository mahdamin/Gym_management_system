# Gym RFID Access Control System

A comprehensive ESP32-based RFID access control system designed for gym facilities. This system provides secure member authentication, real-time monitoring, and web-based management through a modern interface.

## Features

### 🔒 Access Control
- **RFID Authentication**: Secure member identification using RC522 RFID reader
- **Multi-level Access**: Support for Member, Trainer, and Admin access levels
- **Real-time Processing**: Instant access grant/deny decisions
- **Access Logging**: Comprehensive logging of all access attempts

### 🌐 Web Interface
- **Modern Dashboard**: Clean, responsive web interface for system management
- **User Management**: Add, edit, and delete gym members
- **Access Logs**: View recent access attempts and member activity
- **System Monitoring**: Real-time status of WiFi, RFID reader, and system health
- **Configuration**: Easy WiFi setup and system configuration

### 📊 Data Management
- **Persistent Storage**: User data and access logs stored in ESP32's NVS (Non-Volatile Storage)
- **SPIFFS Support**: Web assets served from ESP32's SPIFFS filesystem
- **Backup & Recovery**: Robust data handling with error recovery

### 🔧 Hardware Integration
- **ESP32 Platform**: Built on the powerful ESP32 microcontroller
- **RC522 RFID Reader**: Industry-standard 13.56MHz RFID/NFC reader
- **WiFi Connectivity**: Dual-mode WiFi (Station/AP) for network flexibility
- **Expandable Design**: Modular architecture for easy hardware additions

## Hardware Requirements

### Core Components
- **ESP32 Development Board** (ESP32-DevKitC or similar)
- **RC522 RFID Reader Module**
- **RFID Cards/Tags** (13.56MHz, ISO14443A compatible)
- **Breadboard and Jumper Wires**
- **5V Power Supply** (recommended)

### Wiring Diagram

```
ESP32 Pin    RC522 Pin    Function
---------    ---------    --------
GPIO23       MOSI         SPI Data Out
GPIO19       MISO         SPI Data In
GPIO18       SCK          SPI Clock
GPIO5        SDA/SS       SPI Chip Select
GPIO22       RST          Reset
3.3V         3.3V         Power
GND          GND          Ground
```

## Software Dependencies

### ESP-IDF Components
- `nvs_flash` - Non-volatile storage
- `esp_wifi` - WiFi functionality
- `esp_http_server` - Web server
- `driver` - Hardware drivers
- `spiffs` - File system
- `json` - JSON parsing

### External Libraries
- **RC522 Library**: RFID reader driver (include in project)

## Installation & Setup

### 1. Environment Setup
```bash
# Install ESP-IDF (version 4.4 or later)
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh
source export.sh
```

### 2. Project Setup
```bash
# Clone or create project directory
mkdir gym-rfid-system
cd gym-rfid-system

# Copy all source files to main/ directory
# Copy CMakeLists.txt to project root
```

### 3. Build & Flash
```bash
# Configure the project
idf.py menuconfig

# Build the project
idf.py build

# Flash to ESP32
idf.py -p /dev/ttyUSB0 flash monitor
```

### 4. Initial Configuration
1. **Power on** the ESP32
2. **Connect to WiFi AP**: Look for "GymRFID_Config" network
3. **Access Web Interface**: Open browser to `192.168.4.1`
4. **Configure WiFi**: Enter your network credentials
5. **System Ready**: Device will reboot and connect to your WiFi

## Usage Guide

### Web Interface Access
- **AP Mode**: `http://192.168.4.1`
- **Station Mode**: `http://[device-ip-address]`

### Default Credentials
- **Admin Username**: `admin`
- **Admin Password**: `gym123456`

### Adding Members
1. Navigate to **Users** section
2. Click **Add New User**
3. Enter member details:
   - Name
   - RFID UID (scan card to auto-populate)
   - Access Level (Member/Trainer/Admin)
4. Save the user

### Monitoring Access
- **Real-time Status**: Dashboard shows current system status
- **Access Logs**: View recent member access attempts
- **User Activity**: Track member usage patterns

## API Documentation

### System Status
```http
GET /api/status
```
Returns system health and connectivity status.

### User Management
```http
GET /api/users          # Get all users
POST /api/users         # Create new user
PUT /api/users/{id}     # Update user
DELETE /api/users/{id}  # Delete user
```

### Access Logs
```http
GET /api/access-log     # Get recent access logs
```

### RFID Cards
```http
GET /api/cards          # Get last detected card
```

### Configuration
```http
GET /api/config         # Get system configuration
POST /api/config        # Update configuration
```

## Architecture Overview

### Component Structure
```
├── main.c              # Main application entry point
├── wifi_manager.*      # WiFi connection and AP mode handling
├── web_server.*        # HTTP server and API endpoints
├── rfid_manager.*      # RC522 RFID reader interface
├── user_manager.*      # User authentication and management
├── storage_manager.*   # NVS data persistence
└── CMakeLists.txt      # Build configuration
```

### Data Flow
1. **RFID Detection**: RC522 detects card and reads UID
2. **User Lookup**: System searches for registered user
3. **Access Decision**: Grant/deny based on user status and access level
4. **Logging**: Record access attempt with timestamp
5. **Web Updates**: Real-time status updates via web interface

## Configuration Options

### WiFi Settings
- **SSID**: Target network name
- **Password**: Network password
- **AP Mode**: Fallback configuration mode

### Access Levels
- **Member (1)**: Basic gym access
- **Trainer (2)**: Enhanced access, training areas
- **Admin (3)**: Full system access and management

### System Parameters
- **Scan Interval**: RFID polling frequency
- **Retry Count**: WiFi connection attempts
- **Log Retention**: Number of access logs to keep

## Troubleshooting

### Common Issues

**WiFi Connection Failed**
- Check SSID and password
- Verify signal strength
- Reset to AP mode if needed

**RFID Not Working**
- Verify wiring connections
- Check power supply (3.3V)
- Test with known-good RFID cards

**Web Interface Not Loading**
- Confirm WiFi connection
- Check IP address
- Clear browser cache

**Users Not Saving**
- Check NVS partition
- Verify sufficient flash memory
- Look for duplicate RFID UIDs

### Debug Commands
```bash
# Monitor serial output
idf.py monitor

# Check partition table
idf.py partition_table

# Erase flash (reset everything)
idf.py erase_flash
```

## Security Considerations

### Access Control
- Change default admin credentials immediately
- Use strong WiFi passwords
- Regularly review user access levels
- Monitor access logs for suspicious activity

### Network Security
- Use WPA2/WPA3 WiFi encryption
- Consider VPN for remote access
- Implement firewall rules if needed
- Regular firmware updates

### Physical Security
- Secure ESP32 device installation
- Protect RFID reader from tampering
- Use tamper-evident enclosures
- Backup configuration regularly

## Customization

### Adding New Features
The modular architecture makes it easy to extend functionality:

- **Additional Sensors**: Temperature, motion, etc.
- **External Communication**: SMS, email notifications
- **Database Integration**: MySQL, PostgreSQL support
- **Mobile App**: REST API ready for mobile development

### Hardware Modifications
- **Multiple Readers**: Support for multiple entry points
- **Display Integration**: LCD/OLED status displays
- **Relay Control**: Door locks, lighting control
- **Backup Power**: Battery backup systems

## Contributing

We welcome contributions! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

### Development Guidelines
- Follow ESP-IDF coding standards
- Add comprehensive comments
- Include error handling
- Test with various hardware configurations

## License

This project is licensed under the MIT License. See LICENSE file for details.

## Support

For questions, issues, or feature requests:

- **GitHub Issues**: [Create an issue](https://github.com/your-repo/gym-rfid-system/issues)
- **Documentation**: Check this README and code comments
- **Community**: Join our discussion forum

## Changelog

### Version 1.0.0
- Initial release
- Basic RFID access control
- Web-based user management
- WiFi configuration
- Access logging
- Multi-level access control

### Roadmap
- [ ] Mobile app development
- [ ] Cloud integration
- [ ] Advanced reporting
- [ ] Multi-site support
- [ ] Integration with gym management systems

---

**Built with ❤️ for the maker community**
