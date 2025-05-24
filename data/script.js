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

let expandSideNav = false;
// Sidebar toggle function
function toggleSidebar() {
    const sidebar = document.getElementById('sidebar');
    const main = document.getElementById('main');
    if (sidebar && main) {
        sidebar.classList.toggle('hidden');
        main.classList.toggle('expanded');
        if (!expandSideNav) {
            expandSideNav = !expandSideNav;
        }
    }
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

    // // Login page logic
    // const loginForm = document.getElementById('loginForm');
    // if (loginForm) {
    //     loginForm.addEventListener('submit', function (event) {
    //         event.preventDefault();
    //         const username = document.getElementById('loginUsername').value;
    //         const password = document.getElementById('loginPassword').value;
    //         if (username === 'root' && password === 'toor') {
    //             localStorage.setItem('isLoggedIn', 'true');
    //             window.location.href = 'index.html';
    //         } else {
    //             const loginError = document.getElementById('loginError');
    //             if (loginError) loginError.style.display = 'block';
    //         }
    //     });
    //     return; // Stop further script execution on login page
    // }

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