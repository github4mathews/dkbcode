function applyTheme(theme) {
    document.body.classList.remove('dark-mode');
    if (theme === 'dark') {
        document.body.classList.add('dark-mode');
    } else if (theme === 'auto') {
        const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
        if (prefersDark) document.body.classList.add('dark-mode');
    }
    // Update icon
    const icon = theme === 'dark' ? '🌙' : theme === 'auto' ? '🖥️' : '🌞';
    const themeIcon = document.getElementById('theme-icon');
    if (themeIcon) themeIcon.textContent = icon;
}

function setTheme(theme) {
    localStorage.setItem('theme', theme);
    applyTheme(theme);
}

// let expandSideNav = false;
// // Sidebar toggle function
// function toggleSidebar() {
//     const sidebar = document.getElementById('sidebar');
//     const main = document.getElementById('main');
//     if (sidebar && main) {
//         sidebar.classList.toggle('hidden');
//         main.classList.toggle('expanded');
//         if (!expandSideNav) {
//             expandSideNav = !expandSideNav;
//         }
//     }
// }

function toggleSidebar() {
    const sidebar = document.getElementById('sidebar');
    sidebar.classList.toggle('hidden');
    document.body.classList.toggle('sidebar-collapsed', sidebar.classList.contains('hidden'));
}

// Cookie helpers
function setCookie(name, value, days) {
    let expires = "";
    if (days) {
        const d = new Date();
        d.setTime(d.getTime() + (days * 24 * 60 * 60 * 1000));
        expires = "; expires=" + d.toUTCString();
    }
    document.cookie = name + "=" + (value || "") + expires + "; path=/";
}
function getCookie(name) {
    const nameEQ = name + "=";
    const ca = document.cookie.split(';');
    for (let i = 0; i < ca.length; i++) {
        let c = ca[i];
        while (c.charAt(0) == ' ') c = c.substring(1, c.length);
        if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length, c.length);
    }
    return null;
}
function eraseCookie(name) {
    document.cookie = name + '=; Max-Age=-99999999; path=/';
}

function attemptLogin(username, password, remember) {
    fetch('/users')
        .then(res => res.json())
        .then(users => {
            const found = users.find(u => u.USER_NAME === username && u.PASSWORD === password);
            if (found) {
                if (remember) {
                    setCookie('esp32_user', username, 7);
                    setCookie('esp32_pass', password, 7);
                } else {
                    eraseCookie('esp32_user');
                    eraseCookie('esp32_pass');
                }
                localStorage.setItem('isLoggedIn', 'true');
                window.location.href = 'index.html';
            } else {
                document.getElementById('loginError').style.display = 'block';
                eraseCookie('esp32_user');
                eraseCookie('esp32_pass');
                localStorage.removeItem('isLoggedIn');
            }
        })
        .catch(() => {
            document.getElementById('loginError').style.display = 'block';
        });
}

// Scan for Wi-Fi networks and populate dropdown
function scanWifi() {
    const statusDiv = document.getElementById('wifi-scan-status');
    statusDiv.textContent = "Scanning for Wi-Fi networks...";
    fetch('/wifi/scan/start')
        .then(() => pollWifiScan());
}

function pollWifiScan() {
    fetch('/wifi/scan')
        .then(res => res.json())
        .then(networks => {
            const statusDiv = document.getElementById('wifi-scan-status');
            if (Array.isArray(networks)) {
                statusDiv.textContent = `Found ${networks.length} network(s).`;
                const select = document.getElementById('ssid');
                select.innerHTML = '';
                networks.forEach(net => {
                    const option = document.createElement('option');
                    option.value = net.ssid;
                    option.textContent = `${net.ssid} (${net.rssi} dBm)${net.secure ? ' 🔒' : ''}`;
                    select.appendChild(option);
                });
            } else if (networks.status === "scanning") {
                statusDiv.textContent = "Scanning for Wi-Fi networks...";
                setTimeout(pollWifiScan, 1000); // poll again in 1s
            } else {
                statusDiv.textContent = "No networks found.";
            }
        })
        .catch(() => {
            document.getElementById('wifi-scan-status').textContent = "Failed to scan Wi-Fi networks.";
        });
}

function showCurrentWifi() {
    fetch('/wifi/info')
        .then(res => res.json())
        .then(info => {
            const ssid = info.SSID ? info.SSID : 'Not set';
            document.getElementById('current-wifi').textContent = `Currently connected to: ${ssid}`;
        })
        .catch(() => {
            document.getElementById('current-wifi').textContent = 'Wi-Fi info not available';
        });
}

function showKnownWifi() {
    fetch('/wifi/known')
        .then(res => res.json())
        .then(list => {
            const div = document.getElementById('known-wifi-list');
            if (!Array.isArray(list) || list.length === 0) {
                div.innerHTML = "<em>No known Wi-Fi networks saved.</em>";
                return;
            }
            div.innerHTML = "<b>Known Wi-Fi Networks:</b><ul>" +
                list.map(net => `<li>${net.SSID}</li>`).join('') + "</ul>";
        });
}

function showMqttSettings() {
    fetch('/mqtt/info')
        .then(res => res.json())
        .then(info => {
            document.getElementById('mqttServer').value = info.MQTT_SERVER || '';
            document.getElementById('mqttUsername').value = info.DEFAULT_USERNAME || '';
            document.getElementById('mqttPassword').value = info.DEFAULT_PWD || '';
            document.getElementById('mqttClientId').value = info.ACL_CLIENT_ID || '';
            document.getElementById('mqttPort').value = info.MQ_PORT || '';
            document.getElementById('mqttWsPort').value = info.WS_PORT || '';
            document.getElementById('mqttPublishTopic').value = (info.PUBLISH_TOPIC || []).join(',');
            document.getElementById('mqttSubscribeTopic').value = (info.SUBSCRIBE_TOPIC || []).join(',');
            document.getElementById('mqttServerCert').value = info.SERVER_CERTIFICATE || '';
        });
}

function showFileManager() {
    fetch('/file/list')
        .then(res => res.json())
        .then(files => {
            const fileGrid = document.getElementById('fileGrid');
            fileGrid.innerHTML = ''; // Clear existing files
            files.forEach(file => {
                const fileItem = document.createElement('div');
                fileItem.className = 'file-item';
                fileItem.innerHTML = `
                    <img src="${file.isFolder ? 'folder-icon.png' : 'file-icon.png'}" alt="${file.name}">
                    <span>${file.name}</span>
                `;
                fileItem.onclick = () => {
                    if (file.isFolder) {
                        // Navigate to folder
                        console.log('Open folder:', file.name);
                    } else {
                        // Download file
                        window.location.href = `/file?name=${encodeURIComponent(file.name)}`;
                    }
                };
                fileGrid.appendChild(fileItem);
            });
        });
}

function uploadFile() {
    const fileInput = document.getElementById('fileInput');
    fileInput.click();
}

function handleFileUpload(input) {
    const file = input.files[0];
    if (!file) return;
    const formData = new FormData();
    formData.append('file', file, file.name);
    fetch('/file', { method: 'POST', body: formData })
        .then(() => showFileManager());
}

function createFolder() {
    const folderName = prompt('Enter folder name:');
    if (!folderName) return;
    fetch(`/file?name=${encodeURIComponent(folderName)}`, { method: 'POST' })
        .then(() => showFileManager());
}

document.addEventListener("DOMContentLoaded", function () {
    // Redirect logic for index.html
    if (
        window.location.pathname.endsWith('index.html') ||
        window.location.pathname === '/' ||
        window.location.pathname === '/index'
    ) {
        if (localStorage.getItem('isLoggedIn') !== 'true') {
            window.location.href = 'login.html';
            return;
        }
    }

    // Redirect logic for login.html
    if (window.location.pathname.endsWith('login.html')) {
        const savedUser = getCookie('esp32_user');
        const savedPass = getCookie('esp32_pass');
        if (savedUser && savedPass) {
            attemptLogin(savedUser, savedPass, true);
        }

        document.getElementById('loginForm').addEventListener('submit', function (event) {
            event.preventDefault();
            const username = document.getElementById('loginUsername').value.trim();
            const password = document.getElementById('loginPassword').value.trim();
            const remember = document.getElementById('rememberMe').checked;
            attemptLogin(username, password, remember);
        });
    }

    // Theme persistence for all pages
    window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', function () {
        if (localStorage.getItem('theme') === 'auto') {
            applyTheme('auto');
        }
    });
    const savedTheme = localStorage.getItem('theme') || 'auto';
    applyTheme(savedTheme);

    // The following code is only for index.html/dashboard pages
    // Sidebar toggle function
    if (document.getElementById("sidebar")) {
        window.toggleSidebar = toggleSidebar;
    }

    // Section switching
    window.showSection = function (sectionId) {
        document.querySelectorAll('.section').forEach(section => section.style.display = "none");
        const section = document.getElementById(sectionId);
        if (section) section.style.display = "block";
    };

    // Wi-Fi settings submission
    const wifiForm = document.getElementById("wifiForm");
    if (wifiForm) {
        wifiForm.addEventListener("submit", function (event) {
            event.preventDefault();
            alert("Wi-Fi credentials saved!");
        });
    }

    const wifiFormEl = document.getElementById('wifiForm');
    if (wifiFormEl) {
        wifiFormEl.addEventListener('submit', function (e) {
            e.preventDefault();
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('wifiPassword').value;
            fetch('/wifi/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
            })
                .then(res => res.json())
                .then(data => {
                    alert(data.status === 'success' ? 'Wi-Fi credentials saved!' : 'Failed to save Wi-Fi!');
                });
        });
    }

    const mqttForm = document.getElementById('mqttForm');
    if (mqttForm) {
        mqttForm.addEventListener('submit', function (e) {
            e.preventDefault();
            const data = {
                server: document.getElementById('mqttServer').value,
                username: document.getElementById('mqttUsername').value,
                password: document.getElementById('mqttPassword').value,
                clientId: document.getElementById('mqttClientId').value,
                port: parseInt(document.getElementById('mqttPort').value, 10),
                wsPort: parseInt(document.getElementById('mqttWsPort').value, 10),
                publishTopic: document.getElementById('mqttPublishTopic').value.split(',').map(s => s.trim()).filter(s => s),
                subscribeTopic: document.getElementById('mqttSubscribeTopic').value.split(',').map(s => s.trim()).filter(s => s),
                serverCert: document.getElementById('mqttServerCert').value
            };
            fetch('/mqtt/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(data)
            })
                .then(res => res.json())
                .then(resp => {
                    document.getElementById('mqtt-save-status').textContent =
                        resp.status === 'success' ? 'MQTT settings saved!' : 'Failed to save!';
                });
        });
    }


    window.showSection = function (sectionId) {
        document.querySelectorAll('.section').forEach(section => section.style.display = "none");
        const section = document.getElementById(sectionId);
        if (section) {
            section.style.display = "block";
            if (sectionId === 'wifi-settings') {
                showCurrentWifi();
                showKnownWifi();
                scanWifi();
            }
            else if (sectionId === 'mqtt-settings') showMqttSettings();
        }
    };

    // Notification dropdown logic
    const notificationBtn = document.getElementById('notificationBtn');
    const notificationDropdown = document.getElementById('notificationDropdown');
    if (notificationBtn && notificationDropdown) {
        notificationBtn.addEventListener('click', function (e) {
            e.stopPropagation();
            notificationDropdown.style.display = notificationDropdown.style.display === 'block' ? 'none' : 'block';
        });
        notificationDropdown.addEventListener('click', function (e) {
            e.stopPropagation(); // Prevent closing when clicking inside dropdown
        });
    }

    // Theme dropdown logic
    const themeBtn = document.getElementById('themeDropdownBtn');
    const themeDropdown = document.getElementById('themeDropdown');
    if (themeBtn && themeDropdown) {
        themeBtn.addEventListener('click', function (e) {
            e.stopPropagation();
            themeDropdown.style.display = themeDropdown.style.display === 'block' ? 'none' : 'block';
        });
        themeDropdown.addEventListener('click', function (e) {
            e.stopPropagation();
        });
        document.querySelectorAll('.theme-option').forEach(opt => {
            opt.addEventListener('click', function () {
                setTheme(this.dataset.theme);
                themeDropdown.style.display = 'none';
            });
        });
    }

    // Profile dropdown logic
    const profileImg = document.getElementById('profileImg');
    const profileDropdown = document.getElementById('profileDropdown');
    if (profileImg && profileDropdown) {
        profileImg.addEventListener('click', function (e) {
            e.stopPropagation();
            profileDropdown.style.display = profileDropdown.style.display === 'block' ? 'none' : 'block';
        });
        profileDropdown.addEventListener('click', function (e) {
            e.stopPropagation();
        });
    }

    // Global click to close all dropdowns
    document.addEventListener('click', function () {
        if (notificationDropdown) notificationDropdown.style.display = 'none';
        if (themeDropdown) themeDropdown.style.display = 'none';
        if (profileDropdown) profileDropdown.style.display = 'none';
    });

    // Logout button handler
    const logoutBtn = document.getElementById('logoutBtn');
    if (logoutBtn) {
        logoutBtn.addEventListener('click', function () {
            eraseCookie('esp32_user');
            eraseCookie('esp32_pass');
            localStorage.removeItem('isLoggedIn');
            window.location.href = 'login.html';
        });
    }

    // ESP32 Status Update
    function updateESP32Status() {
        if (
            document.getElementById('cpu-usage') &&
            document.getElementById('ram-usage') &&
            document.getElementById('storage-usage')
        ) {
            fetch('https://mocki.io/v1/0c133372-4f90-4f28-8eb4-aad751251a5f')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('cpu-usage').textContent = data.cpu + '%';
                    document.getElementById('ram-usage').textContent = `${data.ram_used} KB / ${data.ram_total} KB`;
                    document.getElementById('storage-usage').textContent = `${data.storage_used} MB / ${data.storage_total} MB`;
                })
                .catch(() => {
                    document.getElementById('cpu-usage').textContent = '--%';
                    document.getElementById('ram-usage').textContent = '-- KB / -- KB';
                    document.getElementById('storage-usage').textContent = '-- MB / -- MB';
                });
        }
    }

    if (window.location.pathname.endsWith('index.html') || window.location.pathname === '/' || window.location.pathname === '/index') {
        // setInterval(updateESP32Status, 5000);
        // updateESP32Status();
    }

    // ESP32 Temperature & Humidity Live Update via Server-Sent Events (SSE)
    if (!!window.EventSource) {
        var source = new EventSource('/events');

        source.addEventListener('open', function (e) {
            console.log("Events Connected");
        }, false);

        source.addEventListener('error', function (e) {
            if (e.target.readyState != EventSource.OPEN) {
                console.log("Events Disconnected");
            }
        }, false);

        source.addEventListener('message', function (e) {
            console.log("message", e.data);
        }, false);

        source.addEventListener('update', function (event) {
            const data = JSON.parse(event.data);
            document.getElementById('rt1').textContent =
                document.getElementById('rh1').textContent =
                document.getElementById('rp1').textContent =
                document.getElementById('rl1').textContent = data.lastEvent;
            document.getElementById('temperature').textContent = data.temperature;
            document.getElementById('humidity').textContent = data.humidity;
            document.getElementById('pressure').textContent = data.pressure;
            document.getElementById('ldr').textContent = data.ldr;
            document.getElementById('cpu-usage').textContent = data.cpu + '%';
            document.getElementById('ram-usage').textContent = `${data.ram_used} KB / ${data.ram_total} KB`;
            document.getElementById('psram-usage').textContent = `${data.psram_used} KB / ${data.psram_total} KB`;
            document.getElementById('storage-usage').textContent = `${data.flash_used} MB / ${data.flash_total} MB`;;
            document.getElementById('spi_ram_total').textContent = data.spi_ram_total;
        });
    }

    // User management logic
    function fetchUsers() {
        fetch('/users')
            .then(res => res.json())
            .then(users => {
                const tbody = document.getElementById('userTable').querySelector('tbody');
                tbody.innerHTML = '';
                users.forEach(user => {
                    const tr = document.createElement('tr');
                    tr.innerHTML = `
                <td>${user.USER_NAME}</td>
                <td>${user.PASSWORD}</td>
                <td>
                  <button onclick="showUpdateModal('${user.USER_NAME}')">Update</button>
                  <button onclick="deleteUser('${user.USER_NAME}')">Delete</button>
                </td>
              `;
                    tbody.appendChild(tr);
                });
            });
    }

    document.getElementById('userForm').addEventListener('submit', function (e) {
        e.preventDefault();
        const username = document.getElementById('username').value.trim();
        const password = document.getElementById('password').value.trim();
        fetch('/users', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `username=${encodeURIComponent(username)}&password=${encodeURIComponent(password)}`
        })
            .then(res => res.json())
            .then(data => {
                alert(data.status === 'success' ? 'User added!' : 'User exists or error!');
                fetchUsers();
            });
    });

    window.showUpdateModal = function (username) {
        document.getElementById('modalUsername').value = username;
        document.getElementById('modalPassword').value = '';
        document.getElementById('updateModal').style.display = 'block';
    };

    document.getElementById('modalSaveBtn').onclick = function () {
        const username = document.getElementById('modalUsername').value;
        const password = document.getElementById('modalPassword').value;
        fetch('/users', {
            method: 'PUT',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `username=${encodeURIComponent(username)}&password=${encodeURIComponent(password)}`
        })
            .then(res => res.json())
            .then(data => {
                alert(data.status === 'success' ? 'Password updated!' : 'User not found or error!');
                document.getElementById('updateModal').style.display = 'none';
                fetchUsers();
            });
    };

    window.deleteUser = function (username) {
        if (!confirm(`Delete user "${username}"?`)) return;
        fetch('/users', {
            method: 'DELETE',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `username=${encodeURIComponent(username)}`
        })
            .then(res => res.json())
            .then(data => {
                alert(data.status === 'success' ? 'User deleted!' : 'User not found or error!');
                fetchUsers();
            });
    };

    // Initial load
    fetchUsers();
});