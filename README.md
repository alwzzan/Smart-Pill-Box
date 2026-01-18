<h1 align="left">🩺 Smart Pill Box</h1>
<h3 align="left">ESP32-Based Medication Reminder System</h3>

<hr>

<h2>📌 Project Overview</h2>
<p>
<strong>Smart Pill Box</strong> is an embedded IoT device designed to help patients follow
their medication schedules accurately and consistently.
It combines real-time reminders, physical interaction tracking, and wireless configuration
to improve medication adherence in a simple and user-friendly way.
</p>

<p>
The system operates independently using a Real-Time Clock (RTC) and provides both
local control via physical buttons and remote control through a Wi-Fi web interface.
</p>

<hr>

<h2>🎯 Key Features</h2>
<ul>
  <li>
    <strong>⏰ Accurate Medication Reminders</strong><br>
    Uses a DS3231 RTC module to trigger alarms at scheduled dose times.
  </li>
  <li>
    <strong>🔔 Audio & Visual Alerts</strong><br>
    A buzzer and OLED display notify the patient when it is time to take medication.
  </li>
  <li>
    <strong>📦 Lid Opening Detection</strong><br>
    A reed switch detects when the pill box lid is opened, confirming that the dose has been taken.
  </li>
  <li>
    <strong>📊 Medication Adherence Tracking</strong><br>
    Each lid opening is logged to help track patient compliance.
  </li>
  <li>
    <strong>🔘 Physical Button Control</strong><br>
    Three buttons allow navigation, confirmation, mute mode, snooze, and Wi-Fi control.
  </li>
  <li>
    <strong>🌐 Wi-Fi Web Interface</strong><br>
    A built-in web server allows users to:
    <ul>
      <li>Set the current time and date</li>
      <li>Add, edit, or remove medication doses</li>
      <li>Enable or disable alarms</li>
    </ul>
    <em>Time editing is protected to prevent accidental changes.</em>
  </li>
  <li>
    <strong>🕒 12-Hour Time Format (AM/PM)</strong><br>
    Used consistently across both the device and web interface.
  </li>
  <li>
    <strong>🔋 Power-Efficient Design</strong><br>
    The OLED display turns off automatically after inactivity while the system continues running.
  </li>
</ul>

<hr>

<h2>🛠️ Hardware Components</h2>
<ul>
  <li>ESP32 microcontroller</li>
  <li>OLED display (128×64, I2C)</li>
  <li>DS3231 Real-Time Clock (RTC)</li>
  <li>Reed switch + magnet (lid detection)</li>
  <li>Buzzer (audio alerts)</li>
  <li>Three push buttons (OK, NEXT, BACK)</li>
</ul>

<hr>

<h2>🧠 System Logic</h2>
<ul>
  <li>When the current time matches a scheduled dose, the alarm is activated.</li>
  <li>Opening the lid immediately stops the alarm and records the dose as taken.</li>
  <li>If the dose is not taken, a 5-minute snooze can be activated.</li>
  <li>All critical system components are initialized at startup with serial debug output.</li>
</ul>

<hr>

<h2>🚀 Technologies Used</h2>
<ul>
  <li>ESP32 (Arduino Framework / PlatformIO)</li>
  <li>C++</li>
  <li>I2C Communication</li>
  <li>SPIFFS (Web Interface)</li>
  <li>ESPAsyncWebServer</li>
  <li>RTClib</li>
  <li>Adafruit SSD1306</li>
</ul>

<hr>

<h2>🎓 Project Purpose</h2>
<p>
This project was developed as a practical application of embedded systems, IoT development,
and medical device prototyping.
</p>
<p>
Its main goal is to reduce missed medication doses and support home healthcare through
a reliable and easy-to-use smart device.
</p>

<hr>

<h2>📂 Repository Structure</h2>

<pre>
SmartPillBox/
├── src/              // Core firmware code
├── data/             // Web interface (HTML)
├── platformio.ini
└── README.md
</pre>
