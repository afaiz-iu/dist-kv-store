#include<stdio.h>
#include<stdlib.h>
#include <sysexits.h>

struct Node {
    int data;
    struct Node* next;
};

struct Node* head = NULL;

void Print() {
    struct Node* temp = head;
    while(temp != NULL) {
        printf("%d\n", temp -> data);
        temp = temp -> next;
    }
    printf("\n");
}

void Insert(int data, int n) {
    struct Node* temp = (struct Node*)malloc(sizeof(struct Node));
    if (temp == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EX_OSERR);
    }
    if (n < 1) {
        fprintf(stderr, "Invalid list location\n");
        exit(EX_OSERR);
    }
    temp->data = data;
    if (n==1) {
        temp->next = head;
        head = temp;
        return;
    }
    struct Node* temp1 = head;
    for(int i=0; i<n-2; i++) {
        temp1 = temp1->next;
    }
    temp->next = temp1->next;
    temp1->next = temp;
    return;
}

void Delete(int n) {
    if (head==NULL) {
        fprintf(stderr, "List empty for deletion\n");
        exit(EX_OSERR);
    }
    struct Node* temp = head;
    struct Node* prev = NULL;
    if (temp->data == n) {
        head = temp->next;
        free(temp);
        return;
    }
    while(temp != NULL) {
        printf("inside delete\ndata: %d  next memory: %p\n", temp->data,temp->next);
        if (temp->data == n) {
            prev->next = temp->next;
            free(temp);
            return;
        }
        prev = temp;
        temp = temp->next;
    }
    printf("Element not found in list for deletion\n");
    return;
}

void Reverse() {
    if(head == NULL) {
        printf("List empty for reversal\n");
        return;
    }
    struct Node *current, *prev, *next;
    current = head;
    prev = NULL;
    while(current != NULL) {
        next = current->next;
        current->next = prev;
        prev=current;
        current = next;
    }
    head = prev;
    return;
}

int main() {
    Insert(21,1);
    Insert(23,2);
    Insert(25,3);
    Insert(22,2);
    Insert(24,3);
    Print();
    Delete(22);
    Print();
    Insert(22,1);
    Print();
    Reverse();
    Print();
    Delete(67);
    return 0;
}