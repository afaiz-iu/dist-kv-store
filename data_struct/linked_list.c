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

void DeleteByValue(int n) { 
    // n -> value
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

void Delete(int n) { 
    // n -> position
    if (head == NULL) {
        fprintf(stderr, "List empty for deletion\n");
        exit(EX_OSERR);
    }
    struct Node* current = head;
    struct Node* next = head->next;
    if (n < 1) return;
    if (n == 1) {
        head = next;
        free(current);
        return;
        
    }
    int len = 0;
    for (int i = 0; i < n-2 && next->next != NULL; i++) {
        next = next->next;
        current = current -> next;
        len++;
    }
    if (n > len+2) return;
    current->next = next->next;
    free(next);
    return;
}

void Reverse() {
    if(head == NULL) {
        fprintf(stderr, "List empty for deletion\n");
        exit(EX_OSERR);
    }
    struct Node *current = head;
    struct Node *prev = NULL; 
    struct Node *next;
    while(current != NULL) {
        next = current->next;
        current->next = prev;
        prev=current;
        current = next;
    }
    head = prev;
    return;
}

void Flush() {
    struct Node *current = head;
    struct Node *next = head;
    while(current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    return;
}

int main() {
    int opt, val, pos;
    while(1) {
        printf("select op:\n1. insert\n2. delete\n3. reverse\n4. flush\n5. print\n6. exit\n");
        scanf("%d", &opt);
        if (opt == 6) return 0;
        if (opt == 1) {
            printf("Enter value and position:\n");
            scanf("%d %d", &val, &pos);
            Insert(val, pos);
            continue;
        }
        else if (opt == 2) {
            printf("Enter position:\n");
            scanf("%d", &pos);
            Delete(pos);
            continue;
        }
        else if (opt == 3) {
            Reverse();
            continue;
        }
        else if (opt == 4) {
            Flush();
            continue;
        }
        else if (opt == 5) {
            Print();
            continue;
        }
        else {
            return 1;
        }
    }
    return 1;
}