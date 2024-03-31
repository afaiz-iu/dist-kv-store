import os
import re

def clean_content(start_pattern, input_file_path, output_file_path):
    with open(input_file_path, 'r', encoding='utf-8') as file:
        content = file.read()

    match = start_pattern.search(content) # find start of pattern ***
    if match:
        start_index = match.start()
        cleaned_content = content[start_index:]
        with open(output_file_path, 'w', encoding='utf-8') as output_file:
            output_file.write(cleaned_content)
        print(f"Cleaned file saved as: {output_file_path}")
    else:
        print(f"Pattern not found in file: {input_file_path}")

def process_directory(input_dir, output_dir):
    start_pattern = re.compile(r"\*\*\*.*\*\*\*") # pattern to search *** .* ***

    os.makedirs(output_dir, exist_ok=True) # create output dir

    for root, dirs, files in os.walk(input_dir): # walk input dir and process each file 
        for file_name in files:
            # remove carriage returns in file names
            clean_file_nm = file_name.replace('\r', '')
            input_file_path = os.path.join(root, file_name)
            relative_path = os.path.relpath(root, input_dir)
            output_file_dir = os.path.join(output_dir, relative_path)
            os.makedirs(output_file_dir, exist_ok=True)
            output_file_path = os.path.join(output_file_dir, clean_file_nm)

            clean_content(start_pattern, input_file_path, output_file_path)

def main():
    input_dir = 'books'
    output_dir = 'clean_books'
    process_directory(input_dir, output_dir)

if __name__ == "__main__":
    main()

