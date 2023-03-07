import os
import re

# Get the list of all the files in the current directory
files = os.listdir()

for file in files:
    # Skip directories
    if os.path.isdir(file):
        continue

    # Split the file name and extension
    base, ext = os.path.splitext(file)

    # Split the base name into words
    words = re.findall(r'[A-Za-z0-9]+', base)

    # Convert the words to lowercase and join them with underscores
    snake_case = '_'.join([word.lower() for word in words])

    # Construct the new file name with the snake case base and the original extension
    new_name = snake_case + ext

    # Rename the file
    os.rename(file, new_name)

print('Done converting filenames to snake case!')
