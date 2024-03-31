import os
import requests
import time
import re

def fix_filename(title):
    return re.sub(r'[\\/*?:"<>|]', "", title.replace(' ', '_'))

def get_book(book_id, category_dir, timeout):
    book_url = f"https://www.gutenberg.org/cache/epub/{book_id}/pg{book_id}.txt" # construct url
    
    try:
        resp = requests.get(book_url)
        resp.raise_for_status()  # raise HTTPError if request fails
        
        # extract book title
        # search "Title: <title_name> \n"
        match = re.search(r'^Title: (.+)$', resp.text, re.MULTILINE)
        if match:
            title = match.group(1)
            new_title = fix_filename(title) # remove special chars replace whitespace with _
            filename = f"{new_title}.txt"
        else:
            filename = f"pg{book_id}.txt" # use book id if search fails
        
        book_file = os.path.join(category_dir, filename) # download file
        
        # write content to download file
        with open(book_file, 'w', encoding='utf-8') as book_file:
            book_file.write(resp.text)
        print(f"Downloaded book ID {book_id} as {filename}")
        
        time.sleep(timeout) # sleep before next download
    except requests.exceptions.RequestException as e:
        print(f"Failed to download book ID {book_id}: {e}")

def main():
    base_dir = "books"
    timeout = 1 # seconds to pause before successive downloads
    
    for category in os.listdir(base_dir):
        ctg_dir = os.path.join(base_dir, category)
        book_ids_file = os.path.join(ctg_dir, 'book_ids.txt')
        
        if os.path.isfile(book_ids_file):
            with open(book_ids_file, 'r') as file:
                book_ids = file.read().splitlines()
                
            for book_id in book_ids:
                get_book(book_id, ctg_dir, timeout)
        else:
            print(f"No book_ids.txt found for category: {category}")

if __name__ == "__main__":
    main()
