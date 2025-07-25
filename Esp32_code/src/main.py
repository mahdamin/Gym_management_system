from flask import Flask, request, jsonify, render_template, send_file, abort, session, redirect, url_for
from flask_cors import CORS
import json
import os
import base64
import hashlib
import secrets
from datetime import datetime, date, timedelta
from werkzeug.utils import secure_filename
from PIL import Image
import io
import logging
from functools import wraps

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = Flask(__name__)
CORS(app)  # Enable CORS for all routes
app.config['JSONIFY_PRETTYPRINT_REGULAR'] = False 
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16MB max file size
app.config['SECRET_KEY'] = secrets.token_hex(16)  # Generate random secret key

# Configuration
USERS_FILE = 'users.json'
ATTENDANCE_FILE = 'attendance.json'
ADMIN_FILE = 'admin.json'
UPLOAD_FOLDER = 'user_images'
ALLOWED_EXTENSIONS = {'png', 'jpg', 'jpeg', 'gif', 'webp'}
MAX_IMAGE_SIZE = (800, 800)  # Max image dimensions
DEFAULT_IMAGE_QUALITY = 85

# ESP32 IP address - Update this to match your ESP32's IP
ESP32_IP = "192.168.137.253"  # Change this to your ESP32's actual IP address

# Admin credentials (in production, use a proper database)
ADMIN_USERNAME = "admin"
ADMIN_PASSWORD = "admin123"  # Change this in production

# Create necessary folders and files
for folder in [UPLOAD_FOLDER]:
    if not os.path.exists(folder):
        os.makedirs(folder)
        logger.info(f"Created folder: {folder}")

def init_admin_file():
    """Initialize admin file with default admin user"""
    if not os.path.exists(ADMIN_FILE):
        admin_data = {
            "username": ADMIN_USERNAME,
            "password": hashlib.sha256(ADMIN_PASSWORD.encode()).hexdigest(),
            "created_at": datetime.now().isoformat()
        }
        with open(ADMIN_FILE, 'w') as f:
            json.dump(admin_data, f, indent=2)
        logger.info("Admin file initialized")

def admin_required(f):
    """Decorator to require admin authentication"""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if 'admin_logged_in' not in session:
            return jsonify({"error": "Admin authentication required"}), 401
        return f(*args, **kwargs)
    return decorated_function

def allowed_file(filename):
    """Check if file extension is allowed"""
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

def generate_secure_filename(rfid_uid):
    """Generate a secure filename based on RFID UID"""
    hash_object = hashlib.sha256(rfid_uid.encode())
    return f"{hash_object.hexdigest()[:16]}.jpg"

def optimize_image(image_data, max_size=MAX_IMAGE_SIZE, quality=DEFAULT_IMAGE_QUALITY):
    """Optimize image size and quality"""
    try:
        if ',' in image_data:
            image_data = image_data.split(',')[1]
        
        image_binary = base64.b64decode(image_data)
        
        with Image.open(io.BytesIO(image_binary)) as img:
            if img.mode in ('RGBA', 'LA', 'P'):
                img = img.convert('RGB')
            
            img.thumbnail(max_size, Image.Resampling.LANCZOS)
            
            output = io.BytesIO()
            img.save(output, format='JPEG', quality=quality, optimize=True)
            return output.getvalue()
            
    except Exception as e:
        logger.error(f"Error optimizing image: {str(e)}")
        return None

def load_users():
    """Load users from JSON file with error handling"""
    try:
        if os.path.exists(USERS_FILE):
            with open(USERS_FILE, 'r', encoding='utf-8') as f:
                return json.load(f)
    except (json.JSONDecodeError, IOError) as e:
        logger.error(f"Error loading users file: {str(e)}")
    return []

def save_users(users):
    """Save users to JSON file with error handling"""
    try:
        with open(USERS_FILE, 'w', encoding='utf-8') as f:
            json.dump(users, f, indent=2, ensure_ascii=False)
        return True
    except IOError as e:
        logger.error(f"Error saving users file: {str(e)}")
        return False

def load_attendance():
    """Load attendance records from JSON file"""
    try:
        if os.path.exists(ATTENDANCE_FILE):
            with open(ATTENDANCE_FILE, 'r', encoding='utf-8') as f:
                return json.load(f)
    except (json.JSONDecodeError, IOError) as e:
        logger.error(f"Error loading attendance file: {str(e)}")
    return []

def save_attendance(attendance_records):
    """Save attendance records to JSON file"""
    try:
        with open(ATTENDANCE_FILE, 'w', encoding='utf-8') as f:
            json.dump(attendance_records, f, indent=2, ensure_ascii=False)
        return True
    except IOError as e:
        logger.error(f"Error saving attendance file: {str(e)}")
        return False

def log_attendance(user, action="check_in"):
    """Log user attendance"""
    try:
        attendance_records = load_attendance()
        
        attendance_record = {
            "rfid_uid": user.get("rfid_uid"),
            "username": user.get("username"),
            "action": action,
            "timestamp": datetime.now().isoformat(),
            "date": str(date.today()),
            "subscription_type": user.get("subscription_type"),
            "sessions_left": user.get("sessions_left")
        }
        
        attendance_records.append(attendance_record)
        save_attendance(attendance_records)
        logger.info(f"Attendance logged for {user.get('username')}: {action}")
        
    except Exception as e:
        logger.error(f"Error logging attendance: {str(e)}")

def calculate_initial_sessions(subscription_type):
    """Calculate initial sessions for new users"""
    session_mapping = {
        'open_month': 'Unlimited',
        '16_sessions_per_month': 16,
        'three_open_months': 'Unlimited (3 months)'
    }
    return session_mapping.get(subscription_type, 0)

def can_use_session_today(user):
    """Check if user can use a session today (max 1 per day)"""
    today = str(date.today())
    last_visit = user.get('last_visit_date')
    return last_visit != today

def update_user_session(user):
    """Update user's session count and last visit date"""
    today = str(date.today())
    subscription_type = user.get('subscription_type')
    
    if subscription_type == '16_sessions_per_month':
        current_sessions = user.get('sessions_left', 16)
        if isinstance(current_sessions, (int, str)) and str(current_sessions).isdigit():
            current_sessions = int(current_sessions)
            if current_sessions > 0:
                user['sessions_left'] = current_sessions - 1
            else:
                user['sessions_left'] = 0
        else:
            user['sessions_left'] = 16
    
    user['last_visit_date'] = today
    return user

def save_user_image(image_data, rfid_uid):
    """Save and optimize base64 image data to file"""
    try:
        if not image_data:
            return None
            
        filename = generate_secure_filename(rfid_uid)
        filepath = os.path.join(UPLOAD_FOLDER, filename)
        
        optimized_image = optimize_image(image_data)
        if not optimized_image:
            logger.error("Failed to optimize image")
            return None
        
        with open(filepath, 'wb') as f:
            f.write(optimized_image)
        
        logger.info(f"Image saved successfully: {filename}")
        return filename
        
    except Exception as e:
        logger.error(f"Error saving image: {str(e)}")
        return None

def validate_user_data(data):
    """Validate user registration data"""
    required_fields = ['username', 'email', 'age', 'password', 'rfid_uid', 'subscription_type']
    
    for field in required_fields:
        if not data.get(field):
            return False, f"Missing required field: {field}"
    
    try:
        age = int(data['age'])
        if age < 1 or age > 120:
            return False, "Age must be between 1 and 120"
    except (ValueError, TypeError):
        return False, "Invalid age format"
    
    email = data['email']
    if '@' not in email or '.' not in email.split('@')[-1]:
        return False, "Invalid email format"
    
    rfid_uid = data['rfid_uid']
    if len(rfid_uid) < 4 or not all(c.isalnum() for c in rfid_uid):
        return False, "Invalid RFID UID format"
    
    return True, "Valid"

def get_user_stats():
    """Get user statistics for admin dashboard"""
    users = load_users()
    attendance_records = load_attendance()
    
    total_users = len(users)
    active_users_today = len(set(record['rfid_uid'] for record in attendance_records 
                                if record['date'] == str(date.today())))
    
    subscription_stats = {}
    for user in users:
        sub_type = user.get('subscription_type', 'Unknown')
        subscription_stats[sub_type] = subscription_stats.get(sub_type, 0) + 1
    
    # Get attendance for last 7 days
    last_7_days = [(date.today() - timedelta(days=i)).strftime('%Y-%m-%d') for i in range(7)]
    daily_attendance = {}
    for day in last_7_days:
        daily_attendance[day] = len(set(record['rfid_uid'] for record in attendance_records 
                                      if record['date'] == day))
    
    return {
        'total_users': total_users,
        'active_users_today': active_users_today,
        'subscription_stats': subscription_stats,
        'daily_attendance': daily_attendance
    }

# Initialize admin file
init_admin_file()

@app.route("/")
def index():
    """Serve the main registration page"""
    return render_template('index.html', esp32_ip=ESP32_IP)

@app.route("/admin")
def admin_login():
    """Serve admin login page"""
    if 'admin_logged_in' in session:
        return redirect(url_for('admin_dashboard'))
    return render_template('admin_login.html')

@app.route("/admin/login", methods=['POST'])
def admin_login_post():
    """Handle admin login"""
    try:
        data = request.get_json()
        username = data.get('username')
        password = data.get('password')
        
        if not username or not password:
            return jsonify({"error": "Username and password required"}), 400
        
        # Hash the provided password
        hashed_password = hashlib.sha256(password.encode()).hexdigest()
        
        # Load admin credentials
        with open(ADMIN_FILE, 'r') as f:
            admin_data = json.load(f)
        
        if (username == admin_data['username'] and 
            hashed_password == admin_data['password']):
            session['admin_logged_in'] = True
            session['admin_username'] = username
            return jsonify({"success": True}), 200
        else:
            return jsonify({"error": "Invalid credentials"}), 401
            
    except Exception as e:
        logger.error(f"Error during admin login: {str(e)}")
        return jsonify({"error": "Login failed"}), 500

@app.route("/admin/logout", methods=['POST'])
def admin_logout():
    """Handle admin logout"""
    session.pop('admin_logged_in', None)
    session.pop('admin_username', None)
    return jsonify({"success": True}), 200

@app.route("/admin/dashboard")
@admin_required
def admin_dashboard():
    """Serve admin dashboard"""
    return render_template('admin_dashboard.html')

@app.route("/admin/api/stats")
@admin_required
def admin_stats():
    """Get admin statistics"""
    try:
        stats = get_user_stats()
        return jsonify(stats), 200
    except Exception as e:
        logger.error(f"Error getting admin stats: {str(e)}")
        return jsonify({"error": "Failed to get statistics"}), 500

@app.route("/admin/api/users")
@admin_required
def admin_get_users():
    """Get all users for admin"""
    try:
        users = load_users()
        # Remove sensitive data
        safe_users = []
        for user in users:
            safe_user = {k: v for k, v in user.items() if k not in ['password']}
            safe_users.append(safe_user)
        return jsonify(safe_users), 200
    except Exception as e:
        logger.error(f"Error getting users: {str(e)}")
        return jsonify({"error": "Failed to get users"}), 500

@app.route("/admin/api/users/<rfid_uid>", methods=['DELETE'])
@admin_required
def admin_delete_user(rfid_uid):
    """Delete a user"""
    try:
        users = load_users()
        users = [user for user in users if user.get('rfid_uid') != rfid_uid]
        
        if save_users(users):
            logger.info(f"User deleted by admin: {rfid_uid}")
            return jsonify({"success": True}), 200
        else:
            return jsonify({"error": "Failed to save changes"}), 500
            
    except Exception as e:
        logger.error(f"Error deleting user: {str(e)}")
        return jsonify({"error": "Failed to delete user"}), 500

@app.route("/admin/api/users/<rfid_uid>/reset_sessions", methods=['POST'])
@admin_required
def admin_reset_user_sessions(rfid_uid):
    """Reset user sessions"""
    try:
        users = load_users()
        
        for i, user in enumerate(users):
            if user.get('rfid_uid') == rfid_uid:
                subscription_type = user.get('subscription_type')
                users[i]['sessions_left'] = calculate_initial_sessions(subscription_type)
                users[i]['last_visit_date'] = None
                users[i]['reset_timestamp'] = datetime.now().isoformat()
                
                if save_users(users):
                    logger.info(f"Sessions reset by admin for user: {rfid_uid}")
                    return jsonify({"success": True}), 200
                else:
                    return jsonify({"error": "Failed to save changes"}), 500
        
        return jsonify({"error": "User not found"}), 404
        
    except Exception as e:
        logger.error(f"Error resetting sessions: {str(e)}")
        return jsonify({"error": "Failed to reset sessions"}), 500

@app.route("/admin/api/attendance")
@admin_required
def admin_get_attendance():
    """Get attendance records"""
    try:
        attendance_records = load_attendance()
        # Get recent records (last 100)
        recent_records = sorted(attendance_records, key=lambda x: x['timestamp'], reverse=True)[:100]
        return jsonify(recent_records), 200
    except Exception as e:
        logger.error(f"Error getting attendance: {str(e)}")
        return jsonify({"error": "Failed to get attendance records"}), 500

@app.route('/user_image/<filename>')
def user_image(filename):
    """Serve user images with improved security and error handling"""
    try:
        filename = secure_filename(filename)
        if not filename or not allowed_file(filename):
            logger.warning(f"Invalid filename requested: {filename}")
            abort(400, "Invalid filename")
        
        filepath = os.path.join(UPLOAD_FOLDER, filename)
        
        if not os.path.exists(filepath):
            logger.warning(f"Image file not found: {filepath}")
            abort(404, "Image not found")
        
        file_size = os.path.getsize(filepath)
        if file_size > 10 * 1024 * 1024:  # 10MB limit
            logger.warning(f"Image file too large: {filename} ({file_size} bytes)")
            abort(413, "File too large")
        
        return send_file(
            filepath, 
            mimetype='image/jpeg',
            as_attachment=False,
            conditional=True,
            max_age=3600
        )
        
    except Exception as e:
        logger.error(f"Error serving image {filename}: {str(e)}")
        abort(500, f"Server error: {str(e)}")

@app.route('/save', methods=['POST'])
def save_user():
    """Save user with improved validation and error handling"""
    try:
        data = request.get_json()
        if not data:
            return 'Bad Request: No data provided', 400
        
        is_valid, validation_message = validate_user_data(data)
        if not is_valid:
            logger.warning(f"Invalid user data: {validation_message}")
            return f'Validation Error: {validation_message}', 400
        
        users = load_users()
        
        rfid_uid = data.get('rfid_uid')
        for existing_user in users:
            if existing_user.get('rfid_uid') == rfid_uid:
                logger.warning(f"Duplicate RFID UID registration attempt: {rfid_uid}")
                return 'Error: RFID card already registered', 400
        
        image_filename = None
        if data.get('image_data'):
            image_filename = save_user_image(data['image_data'], rfid_uid)
            if image_filename:
                data['image_filename'] = image_filename
                logger.info(f"Image saved for user: {image_filename}")
            else:
                logger.warning("Failed to save user image")
            del data['image_data']
        
        if 'sessions_left' not in data:
            data['sessions_left'] = calculate_initial_sessions(data.get('subscription_type'))
        
        data['registration_timestamp'] = datetime.now().isoformat()
        
        users.append(data)
        
        if not save_users(users):
            return 'Error: Failed to save user data', 500
        
        logger.info(f"New user registered: {data.get('username')} (UID: {rfid_uid}) with {data.get('sessions_left')} sessions" + 
                   (f" and image: {image_filename}" if image_filename else ""))
        return 'User registered successfully', 200
        
    except Exception as e:
        logger.error(f"Error saving user: {str(e)}")
        return f'Internal Server Error: {str(e)}', 500

@app.route('/check_user', methods=['POST'])
def check_user():
    """Check user with improved error handling and security"""
    try:
        data = request.get_json()
        if not data or 'rfid_uid' not in data:
            return 'Bad Request: No RFID UID provided', 400
        
        uid = data['rfid_uid']
        
        if not uid or len(uid) < 4:
            logger.warning(f"Invalid RFID UID format: {uid}")
            return 'Bad Request: Invalid RFID UID format', 400
        
        users = load_users()
        user_found = False
        
        for i, user in enumerate(users):
            if user.get('rfid_uid') == uid:
                user_found = True
                subscription_type = user.get('subscription_type')
                image_filename = user.get('image_filename')
                
                # Log attendance
                log_attendance(user, "check_in")
                
                if subscription_type == '16_sessions_per_month':
                    current_sessions = user.get('sessions_left', 16)
                    if isinstance(current_sessions, str) and current_sessions.isdigit():
                        current_sessions = int(current_sessions)
                    
                    if isinstance(current_sessions, int) and current_sessions <= 0:
                        response_data = {
                            "scanned": True,
                            "uid": uid,
                            "registered": True,
                            "username": user.get("username", "Unknown"),
                            "subscription_type": subscription_type,
                            "sessions_left": "No sessions left",
                            "image_filename": image_filename
                        }
                        logger.info(f"User has no sessions left: {uid}")
                        return jsonify(response_data), 200
                    
                    if not can_use_session_today(user):
                        response_data = {
                            "scanned": True,
                            "uid": uid,
                            "registered": True,
                            "username": user.get("username", "Unknown"),
                            "subscription_type": subscription_type,
                            "sessions_left": f"{current_sessions} (Already used today)",
                            "image_filename": image_filename
                        }
                        logger.info(f"User already visited today: {uid}")
                        return jsonify(response_data), 200
                    
                    users[i] = update_user_session(user)
                    if not save_users(users):
                        logger.error("Failed to save updated user data")
                        return 'Error: Failed to update user data', 500
                    
                    sessions_left = users[i].get('sessions_left')
                    response_data = {
                        "scanned": True,
                        "uid": uid,
                        "registered": True,
                        "username": user.get("username", "Unknown"),
                        "subscription_type": subscription_type,
                        "sessions_left": str(sessions_left),
                        "image_filename": image_filename
                    }
                    logger.info(f"Session used - User: {uid}, Sessions left: {sessions_left}")
                    return jsonify(response_data), 200
                else:
                    users[i]['last_visit_date'] = str(date.today())
                    if not save_users(users):
                        logger.error("Failed to save updated user data")
                    
                    sessions_left = user.get('sessions_left', calculate_initial_sessions(subscription_type))
                    response_data = {
                        "scanned": True,
                        "uid": uid,
                        "registered": True,
                        "username": user.get("username", "Unknown"),
                        "subscription_type": subscription_type,
                        "sessions_left": str(sessions_left),
                        "image_filename": image_filename
                    }
                    logger.info(f"Unlimited user visited: {uid}")
                    return jsonify(response_data), 200
        
        if not user_found:
            logger.info(f"User not found for UID: {uid}")
            return jsonify({"scanned": True, "uid": uid, "registered": False}), 200
            
    except Exception as e:
        logger.error(f"Error checking user: {str(e)}")
        return f'Internal Server Error: {str(e)}', 500

@app.route("/renew_subscription", methods=["POST"])
def renew_subscription():
    """Renew subscription with improved validation"""
    try:
        data = request.get_json()
        if not data or "rfid_uid" not in data or "new_subscription_type" not in data:
            return "Bad Request: Missing RFID UID or new subscription type", 400
        
        uid = data["rfid_uid"]
        new_subscription_type = data["new_subscription_type"]
        
        valid_subscriptions = ['open_month', '16_sessions_per_month', 'three_open_months']
        if new_subscription_type not in valid_subscriptions:
            return "Bad Request: Invalid subscription type", 400
        
        users = load_users()
        
        for i, user in enumerate(users):
            if user.get("rfid_uid") == uid:
                users[i]["subscription_type"] = new_subscription_type
                users[i]["sessions_left"] = calculate_initial_sessions(new_subscription_type)
                users[i]["last_visit_date"] = None
                users[i]["renewal_timestamp"] = datetime.now().isoformat()
                
                if not save_users(users):
                    return "Error: Failed to save renewal data", 500
                
                # Log renewal activity
                log_attendance(users[i], "subscription_renewal")
                
                logger.info(f"User {user.get('username')} (UID: {uid}) renewed subscription to {new_subscription_type}")
                return f"Subscription renewed successfully for {user.get('username')}", 200
        
        return "User not found", 404
        
    except Exception as e:
        logger.error(f"Error renewing subscription: {str(e)}")
        return f"Internal Server Error: {str(e)}", 500

@app.route('/users', methods=['GET'])
def get_all_users():
    """Debug endpoint to view all users (consider removing in production)"""
    try:
        users = load_users()
        safe_users = []
        for user in users:
            safe_user = {k: v for k, v in user.items() if k not in ['password', 'image_data']}
            safe_users.append(safe_user)
        return jsonify(safe_users), 200
    except Exception as e:
        logger.error(f"Error retrieving users: {str(e)}")
        return f'Error: {str(e)}', 500

@app.route('/reset_user_sessions', methods=['POST'])
def reset_user_sessions():
    """Admin endpoint to reset a user's sessions"""
    try:
        data = request.get_json()
        if not data or 'rfid_uid' not in data:
            return 'Bad Request: No RFID UID provided', 400
        
        uid = data['rfid_uid']
        users = load_users()
        
        for i, user in enumerate(users):
            if user.get('rfid_uid') == uid:
                subscription_type = user.get('subscription_type')
                users[i]['sessions_left'] = calculate_initial_sessions(subscription_type)
                users[i]['last_visit_date'] = None
                users[i]['reset_timestamp'] = datetime.now().isoformat()
                
                if not save_users(users):
                    return 'Error: Failed to save reset data', 500
                
                logger.info(f"Sessions reset for user: {user.get('username')} (UID: {uid})")
                return f"Sessions reset for user: {user.get('username')}", 200
        
        return 'User not found', 404
        
    except Exception as e:
        logger.error(f"Error resetting sessions: {str(e)}")
        return f'Error: {str(e)}', 500

@app.errorhandler(404)
def not_found_error(error):
    return jsonify({"error": "Not found"}), 404

@app.errorhandler(500)
def internal_error(error):
    logger.error(f"Internal server error: {str(error)}")
    return jsonify({"error": "Internal server error"}), 500

if __name__ == '__main__':
    logger.info("=== Enhanced Gym Registration Server ===")
    logger.info("Features:")
    logger.info("- Admin panel with user management")
    logger.info("- Attendance tracking and analytics")
    logger.info("- Enhanced security with session management")
    logger.info("- Improved image handling with optimization")
    logger.info("- Better error handling and logging")
    logger.info("- CORS enabled for cross-origin requests")
    logger.info("- Responsive design with mobile support")
    logger.info(f"- ESP32 IP configured: {ESP32_IP}")
    logger.info("- Secure filename generation")
    logger.info("- File type and size validation")
    logger.info("=======================================")
    logger.info("Access the web interface at: http://localhost:5000")
    logger.info("Access the admin panel at: http://localhost:5000/admin")
    logger.info(f"Admin credentials: {ADMIN_USERNAME} / {ADMIN_PASSWORD}")
    
    app.run(host='0.0.0.0', port=5000, debug=False)

