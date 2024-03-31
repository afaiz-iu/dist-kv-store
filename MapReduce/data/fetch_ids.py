# code to parse project gutenberg html and fetch book ids for a given category
# mention the category and url in bookshelves.json
import requests
from bs4 import BeautifulSoup
import os
import json

def fetch_book_ids(bookshelf_url):
    response = requests.get(bookshelf_url)
    if response.status_code != 200:
        print("Failed to fetch the bookshelf page")
        return []

    soup = BeautifulSoup(response.text, 'html.parser')
    book_links = soup.find_all('a', href=True, class_='link') # get all links from bookshelf
    book_ids = []

    for link in book_links:
        href = link['href']
        if href.startswith('/ebooks/'): # filter for /ebooks/
            book_id = href.split('/')[-1]
            if book_id.isdigit():
                book_ids.append(book_id)

    return book_ids

def create_output_directories(base_dir, bookshelves):
    for shelf_name, shelf_url in bookshelves.items():
        dir_ = os.path.join(base_dir, shelf_name)
        os.makedirs(dir_, exist_ok=True)
        
        book_ids = fetch_book_ids(shelf_url) # fetch book ids
        
        with open(os.path.join(dir_, 'book_ids.txt'), 'w') as file:
            for book_id in book_ids:
                file.write(book_id + '\n') # write to txt
        print("{} written to disk".format(shelf_name))

def main():
    # get bookshelf urls
    with open('bookshelves.json', 'r') as file:
        bookshelves = json.load(file)
    
    base_dir = os.path.join(os.getcwd(), 'books')
    create_output_directories(base_dir, bookshelves)

if __name__ == "__main__":
    main()
