function showFileManager() {
    fetch('/file/list')
        .then(res => res.json())
        .then(files => {
            const fileGrid = document.getElementById('fileGrid');
            fileGrid.innerHTML = '';
            files.forEach(file => {
                const fileItem = document.createElement('div');
                fileItem.className = 'file-item';
                fileItem.innerHTML = `
                    <img src="${file.isDir ? 'folder-icon.png' : 'file-icon.png'}" alt="${file.name}">
                    <span>${file.name}</span>
                `;
                fileItem.onclick = () => {
                    if (file.isDir) {
                        console.log('Open folder:', file.name);
                    } else {
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
    fetch(`/folder?name=${encodeURIComponent(folderName)}`, { method: 'POST' })
        .then(res => res.text())
        .then(msg => {
            alert(msg);
            showFileManager();
        });
}

function saveEdit() {
    const content = document.getElementById('editorArea').value;
    fetch(`/file/edit?name=${encodeURIComponent(currentEditFile)}`, {
        method: "PUT",
        headers: { "Content-Type": "text/plain" },
        body: content
    }).then(res => res.text())
        .then(msg => {
            alert(msg);
            closeEditor();
            showFileManager();
        });
}

document.addEventListener('DOMContentLoaded', showFileManager);