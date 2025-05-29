# import os
# import gzip
# import shutil

# def compress_files(source_dir):
#     for root, _, files in os.walk(source_dir):
#         for file in files:
#             file_path = os.path.join(root, file)
#             gz_path = file_path + ".gz"
#             with open(file_path, 'rb') as f_in:
#                 with gzip.open(gz_path, 'wb') as f_out:
#                     shutil.copyfileobj(f_in, f_out)

# def before_upload(source, target, env):
#     data_dir = os.path.join(env['PROJECT_DIR'], 'data')
#     compress_files(data_dir)

# import os
# import subprocess

# def compress_files(source_dir):
#     # Path to Git Bash executable
#     git_bash_path = "C:\\Program Files\\Git\\bin\\bash.exe"

#     # Command to recursively gzip all files in the source directory
#     gzip_command = f'find "{source_dir}" -type f -exec gzip -k {{}} \\;'

#     # Run the command using Git Bash
#     try:
#         subprocess.run([git_bash_path, "-c", gzip_command], check=True)
#         print(f"Successfully compressed files in {source_dir}")
#     except subprocess.CalledProcessError as e:
#         print(f"Error compressing files: {e}")

# def before_upload(source, target, env):
#     # Path to the 'data' directory
#     data_dir = os.path.join(env["PROJECT_DIR"], "data")
#     compress_files(data_dir)

import os
import subprocess

def compress_files(source_dir):
    # Path to Git Bash executable
    git_bash_path = "C:\\Program Files\\Git\\bin\\bash.exe"

    # Command to recursively gzip all files in the source directory
    gzip_command = f'find "{source_dir}" -type f -exec gzip -k {{}} \\;'

    # Run the command using Git Bash
    try:
        print(f"Running gzip command: {gzip_command}")
        subprocess.run([git_bash_path, "-c", gzip_command], check=True)
        print(f"Successfully compressed files in {source_dir}")
    except subprocess.CalledProcessError as e:
        print(f"Error compressing files: {e}")

def before_upload(source, target, env):
    # Path to the 'data' directory
    data_dir = os.path.join(env["PROJECT_DIR"], "data")
    if os.path.exists(data_dir):
        compress_files(data_dir)
    else:
        print(f"Data directory not found: {data_dir}")