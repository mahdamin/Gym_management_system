# Gym Management System with RFID Access Control

A comprehensive Flask-based gym management system featuring RFID card access control, user registration, subscription management, and administrative dashboard with attendance tracking.

## Features

### Core Functionality
- **RFID-based Access Control**: Users can check in using RFID cards
- **User Registration**: Web-based registration with image upload
- **Subscription Management**: Multiple subscription types with session tracking
- **Admin Dashboard**: Complete administrative interface
- **Attendance Tracking**: Real-time attendance logging and analytics
- **Image Management**: User photo storage with optimization

### Subscription Types
1. **Open Month**: Unlimited access for one month
2. **16 Sessions per Month**: Limited to 16 sessions with daily usage restrictions
3. **Three Open Months**: Unlimited access for three months

### Security Features
- Admin authentication with session management
- Secure image filename generation
- Input validation and sanitization
- CORS support for cross-origin requests
- File type and size validation

## Technology Stack

- **Backend**: Flask (Python)
- **Frontend**: HTML, CSS, JavaScript
- **Database**: JSON file storage
- **Image Processing**: Pillow (PIL)
- **Authentication**: Session-based admin authentication
- **Hardware**: ESP32 for RFID scanning

## Installation

### Prerequisites
- Python 3.7+
- ESP32 with RFID reader (optional for testing)

### Setup Instructions

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd gym-management-system
   ```

2. **Install dependencies**
   ```bash
   pip install -r requirements.txt
   ```

3. **Configure ESP32 IP (Optional)**
   Edit the `ESP32_IP` variable in `main.py` or `server.py`:
   ```python
   ESP32_IP = "192.168.137.253"  # Change to your ESP32's IP
   ```

4. **Run the application**
   ```bash
   python main.py
   # or
   python server.py
   ```

5. **Access the application**
   - Main interface: http://localhost:5000
   - Admin panel: http://localhost:5000/admin

## File Structure

```
gym-management-system/
├── main.py                 # Main Flask application
├── server.py              # Alternative server file (identical to main.py)
├── requirements.txt       # Python dependencies
├── admin.json            # Admin credentials (auto-generated)
├── users.json            # User database (auto-generated)
├── attendance.json       # Attendance records (auto-generated)
├── user_images/          # User photos directory (auto-created)
└── templates/            # HTML templates (not included in provided files)
    ├── index.html
    ├── admin_login.html
    └── admin_dashboard.html
```

## API Endpoints

### Public Endpoints
- `GET /` - Main registration page
- `POST /save` - Register new user
- `POST /check_user` - Check user by RFID UID
- `POST /renew_subscription` - Renew user subscription
- `GET /user_image/<filename>` - Serve user images
- `GET /users` - Get all users (debug endpoint)
- `POST /reset_user_sessions` - Reset user sessions

### Admin Endpoints
- `GET /admin` - Admin login page
- `POST /admin/login` - Admin authentication
- `POST /admin/logout` - Admin logout
- `GET /admin/dashboard` - Admin dashboard
- `GET /admin/api/stats` - Get system statistics
- `GET /admin/api/users` - Get all users
- `DELETE /admin/api/users/<rfid_uid>` - Delete user
- `POST /admin/api/users/<rfid_uid>/reset_sessions` - Reset user sessions
- `GET /admin/api/attendance` - Get attendance records

## Configuration

### Admin Credentials
Default admin credentials (change in production):
- **Username**: admin
- **Password**: admin123

The password is automatically hashed using SHA-256 when stored.

### Image Settings
```python
MAX_IMAGE_SIZE = (800, 800)  # Maximum image dimensions
DEFAULT_IMAGE_QUALITY = 85   # JPEG quality (1-100)
ALLOWED_EXTENSIONS = {'png', 'jpg', 'jpeg', 'gif', 'webp'}
```

### File Size Limits
- Maximum upload size: 16MB
- Maximum image file size when serving: 10MB

## Usage

### User Registration
1. Access the main page at http://localhost:5000
2. Fill in user details including RFID UID
3. Select subscription type
4. Upload user photo (optional)
5. Submit registration

### RFID Check-in
Send POST request to `/check_user` with RFID UID:
```json
{
  "rfid_uid": "2321E505"
}
```

Response includes user info and session status:
```json
{
  "scanned": true,
  "uid": "2321E505",
  "registered": true,
  "username": "John Doe",
  "subscription_type": "16_sessions_per_month",
  "sessions_left": "15",
  "image_filename": "abc123def456.jpg"
}
```

### Admin Dashboard
1. Navigate to http://localhost:5000/admin
2. Login with admin credentials
3. View statistics, manage users, and monitor attendance

## Data Storage

The system uses JSON files for data persistence:

### users.json
Stores user information including:
- Personal details (username, email, age)
- RFID UID
- Subscription type and sessions remaining
- Registration timestamp
- Image filename

### attendance.json
Logs all user activities:
- Check-ins and subscription renewals
- Timestamps and dates
- User identification
- Session information

### admin.json
Stores admin credentials (hashed password)

## Security Considerations

### Production Deployment
1. **Change default admin password**
2. **Use environment variables** for sensitive configuration
3. **Implement HTTPS** for secure communication
4. **Set up proper database** instead of JSON files
5. **Add rate limiting** to prevent abuse
6. **Implement backup system** for data files

### Current Security Features
- Password hashing (SHA-256)
- Secure filename generation
- File type validation
- Input sanitization
- Session-based authentication
- CORS configuration

## ESP32 Integration

The system is designed to work with an ESP32 microcontroller equipped with an RFID reader. The ESP32 should:

1. Read RFID cards
2. Send HTTP POST requests to `/check_user`
3. Display user information and access status
4. Handle network connectivity

## Troubleshooting

### Common Issues

1. **Module Import Errors**
   ```bash
   pip install -r requirements.txt
   ```

2. **Permission Errors**
   Ensure the application has write permissions for:
   - Current directory (for JSON files)
   - `user_images/` directory

3. **Image Upload Issues**
   - Check file size limits
   - Verify supported file formats
   - Ensure proper base64 encoding

4. **ESP32 Connection**
   - Verify ESP32 IP address in configuration
   - Check network connectivity
   - Ensure firewall allows connections

### Debug Mode
Enable debug mode for development:
```python
app.run(host='0.0.0.0', port=5000, debug=True)
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## License

This project is open source. Please check the license file for details.

## Support

For issues and questions:
1. Check the troubleshooting section
2. Review the API documentation
3. Submit an issue on the repository

## Future Enhancements

- Database migration (SQLite/PostgreSQL)
- User authentication and self-service portal
- Mobile application
- Advanced reporting and analytics
- Integration with payment systems
- Multi-gym support
- Email notifications
- Backup and restore functionality
