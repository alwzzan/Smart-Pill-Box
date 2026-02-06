<h1 align="left">🩺 Smart Pill Box</h1>
<h3 align="left">ESP32-Based Smart Medication Reminder System</h3>

<hr>

<h2>📌 Project Overview</h2>
<p>
<strong>Smart Pill Box</strong> is an embedded IoT medication reminder device designed to help patients follow their medication schedules accurately and consistently.
It combines real-time reminders, interaction tracking, and wireless configuration to improve medication adherence in a simple and user-friendly way.
</p>

<p>
The system operates independently using a Real-Time Clock (RTC) while providing both local control through physical buttons and remote configuration via a Wi-Fi web interface.
Recent updates significantly improved system stability, storage reliability, and web interface performance.
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
    Buzzer and OLED display notify the user when medication time arrives.
  </li>

  <li>
    <strong>📦 Lid Opening Detection</strong><br>
    Reed switch detects lid opening and confirms medication access.
  </li>

  <li>
    <strong>📊 Lid Opening Counter</strong><br>
    Tracks real lid openings independently from dose tracking, allowing better compliance analysis.
  </li>

  <li>
    <strong>💾 Persistent Storage</strong><br>
    Medication schedules and usage data are saved in ESP32 memory and remain available after power loss or restart.
  </li>

  <li>
    <strong>🔘 Physical Button Control</strong><br>
    Three buttons allow menu navigation, confirmation, mute mode, snooze, and Wi-Fi control.
  </li>

  <li>
    <strong>🌐 Stable Wi-Fi Web Interface</strong><br>
    Built-in web server allows users to:
    <ul>
      <li>Set time and date</li>
      <li>Add, edit, or delete doses</li>
      <li>Enable or disable alarms</li>
    </ul>
    <em>Wi-Fi and server logic redesigned for stable operation.</em>
  </li>

  <li>
    <strong>⚡ Fast & Reliable Web UI</strong><br>
    Web interface rebuilt to load quickly and reliably without freezing or hanging.
  </li>

  <li>
    <strong>🕒 Flexible Dose Scheduling</strong><br>
    Multiple doses can now be scheduled minutes apart without artificial spacing limits.
  </li>

  <li>
    <strong>📱 Improved Web Experience</strong><br>
    Modal dialogs and UI interactions are fixed to prevent stuck screens or blocked navigation.
  </li>

  <li>
    <strong>🕒 12-Hour Time Format</strong><br>
    Consistent AM/PM format across device and web interface.
  </li>

  <li>
    <strong>🔋 Power-Efficient Design</strong><br>
    OLED screen automatically turns off after inactivity while system remains active.
  </li>
</ul>

<hr>

<h2>🛠️ Hardware Components</h2>
<ul>
  <li>ESP32 microcontroller</li>
  <li>OLED display (128×64 I2C)</li>
  <li>DS3231 Real-Time Clock</li>
  <li>Reed switch + magnet</li>
  <li>Buzzer</li>
  <li>Three push buttons</li>
</ul>

<hr>

<h2>🧠 System Logic</h2>
<ul>
  <li>Alarm triggers when scheduled dose time is reached.</li>
  <li>Opening the lid stops the alarm and marks dose as taken.</li>
  <li>Snooze can delay reminders when needed.</li>
  <li>Lid openings are tracked separately from doses.</li>
  <li>System settings and schedules persist after restart.</li>
</ul>

<hr>

<h2>🚀 Technologies Used</h2>
<ul>
  <li>ESP32 (Arduino Framework / PlatformIO)</li>
  <li>C++</li>
  <li>I2C Communication</li>
  <li>SPIFFS Filesystem</li>
  <li>ESPAsyncWebServer</li>
  <li>RTClib</li>
  <li>Adafruit SSD1306</li>
</ul>

<hr>

<h2>🎓 Project Purpose</h2>
<p>
This project demonstrates a practical application of embedded systems and IoT in healthcare support.
It aims to reduce missed medication doses and provide a reliable smart home healthcare assistant.
</p>

<hr>

<h2>📂 Repository Structure</h2>

<pre>
SmartPillBox/
├── src/              // Firmware source code
├── data/             // Web interface files
├── platformio.ini
└── README.md
</pre>
