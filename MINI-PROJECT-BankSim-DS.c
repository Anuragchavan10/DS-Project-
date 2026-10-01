/*
 * Bank Account Simulation - Data Structures Mini Project
 *
 * Data structures used:
 *   1. struct User         -> one customer RECORD
 *   2. File of records     -> bank_database.dat (array of fixed-size records on disk)
 *   3. Stack (linked list) -> transaction history, LIFO (last transaction on top)
 *
 * Transactions are also saved in transactions.dat, so history survives restarts.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HISTORY_SHOW 5   // "last N transactions" = N

// ---------------------------------------------------------------
// 1. RECORD: one customer (one row of our file "database")
// ---------------------------------------------------------------
struct User {
    int id;             // Unique Customer ID
    char password[20];  // Authentication Key
    char name[50];      // Customer Name
    char pan[15];       // Customer PAN card Number
    char phone[15];     // Customer Phone Number
    float balance;      // Account Funds
};

// ---------------------------------------------------------------
// 2. STACK of transactions (implemented as a singly linked list)
// ---------------------------------------------------------------
struct Transaction {        // one history entry (also one record in transactions.dat)
    int id;                 // which customer it belongs to
    char type;              // 'D' = deposit, 'W' = withdraw
    float amount;           // amount moved
    float balanceAfter;     // balance after this transaction
};

struct Node {               // one stack node
    struct Transaction data;
    struct Node *next;      // points to the node BELOW it
};

struct Stack {
    struct Node *top;       // only the top is ever accessed
    int size;
};

// --- Stack operations (all O(1) except clearStack) ---
void initStack(struct Stack *s) {
    s->top = NULL;
    s->size = 0;
}

int isEmpty(struct Stack *s) {
    return s->top == NULL;
}

void push(struct Stack *s, struct Transaction t) {
    struct Node *n = (struct Node *)malloc(sizeof(struct Node));
    if (n == NULL) return;          // out of memory
    n->data = t;
    n->next = s->top;               // new node sits on the old top
    s->top = n;                     // new node becomes the top
    s->size++;
}

int pop(struct Stack *s, struct Transaction *out) {
    if (isEmpty(s)) return 0;       // underflow
    struct Node *n = s->top;
    *out = n->data;                 // give the value back
    s->top = n->next;               // top moves down
    free(n);
    s->size--;
    return 1;
}

int peek(struct Stack *s, struct Transaction *out) {
    if (isEmpty(s)) return 0;
    *out = s->top->data;            // look at the top, do not remove
    return 1;
}

void clearStack(struct Stack *s) {  // free every node (O(n))
    struct Transaction dummy;
    while (pop(s, &dummy)) { }
}

// ---------------------------------------------------------------
// Function prototypes
// ---------------------------------------------------------------
void clearScreen();
void printHeader(const char *title);
int idExists(int id);
void createAccount();
void login();
void userMenu(struct User currentUser);
void updateBalance(int id, float newBalance);
void showUsers();
void loadHistory(int id, struct Stack *s);
void recordTransaction(struct Stack *s, int id, char type, float amount, float balanceAfter);
void showHistory(struct Stack *s, int n);

// ---------------------------------------------------------------
// Main: menu loop
// ---------------------------------------------------------------
int main() {
    int choice;
    srand(time(0));

    while (1) {
        clearScreen();
        printHeader("WELCOME TO STUDENT BANK SYSTEM");
        printf(" 1. Login to Existing Account\n");
        printf(" 2. Create New Account\n");
        printf(" 3. Exit\n");
        printf("====================================================\n");
        printf(" Enter Choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: login();
                break;
            case 2: createAccount();
                break;
            case 3:
                printf("\n Thank you for banking with us!\n");
                exit(0);
            case 0:
                showUsers();
                break;
            default:
                printf("\n Invalid Option! Press Enter to retry...");
                getchar();
                getchar();
        }
    }
    return 0;
}

// ---------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void printHeader(const char *title) {
    printf("\n====================================================\n");
    printf("        %s\n", title);
    printf("====================================================\n");
}

// Linear search: is this ID already used?  O(n)
int idExists(int id) {
    struct User u;
    FILE *fp = fopen("bank_database.dat", "rb");
    if (fp == NULL) return 0;
    while (fread(&u, sizeof(struct User), 1, fp)) {
        if (u.id == id) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// INSERT: append a new record at the end of the file  O(1) (+ O(n) uniqueness check)
void createAccount() {
    struct User u;
    FILE *fp;

    clearScreen();
    printHeader("OPEN NEW ACCOUNT");

    printf(" Enter Your Full Name: ");
    scanf(" %49[^\n]", u.name);       // width limit prevents buffer overflow

    printf(" Enter PAN Number: ");
    scanf("%14s", u.pan);

    printf(" Enter Phone Number: ");
    scanf("%14s", u.phone);

    printf(" Create a Password: ");
    scanf("%19s", u.password);

    // Generate a random 5-digit ID that is not already taken
    do {
        u.id = 10000 + rand() % 90000;
    } while (idExists(u.id));
    u.balance = 0.0;

    fp = fopen("bank_database.dat", "ab");   // append binary
    if (fp == NULL) {
        printf("\n Error accessing database!\n");
        return;
    }
    fwrite(&u, sizeof(struct User), 1, fp);
    fclose(fp);

    printf("\n [SUCCESS] Account Created Successfully!");
    printf("\n YOUR CUSTOMER ID: %d", u.id);
    printf("\n (Please remember this ID for login)");
    printf("\n\n Press Enter to return to Main Menu...");
    getchar(); getchar();
}

// SEARCH: linear search through the file by ID + password  O(n)
void login() {
    int inputID, found = 0;
    char inputPass[20];
    struct User u;
    FILE *fp;

    clearScreen();
    printHeader("SECURE LOGIN");

    printf(" Enter Customer ID: ");
    scanf("%d", &inputID);

    printf(" Enter Password: ");
    scanf("%19s", inputPass);

    fp = fopen("bank_database.dat", "rb");
    if (fp == NULL) {
        printf("\n [ERROR] No accounts found in system.\n Press Enter...");
        getchar(); getchar();
        return;
    }

    while (fread(&u, sizeof(struct User), 1, fp)) {
        if (u.id == inputID && strcmp(u.password, inputPass) == 0) {
            found = 1;
            break;
        }
    }
    fclose(fp);

    if (found) {
        printf("\n [SUCCESS] Login Verified. Redirecting...");
        userMenu(u);
    } else {
        printf("\n [FAILED] Invalid ID or Password.\n Press Enter to retry...");
        getchar(); getchar();
    }
}

// ---------------------------------------------------------------
// Dashboard: owns one transaction STACK for the logged-in user
// ---------------------------------------------------------------
void userMenu(struct User currentUser) {
    int choice;
    float amount;
    struct Stack history;

    initStack(&history);
    loadHistory(currentUser.id, &history);   // rebuild the stack from file

    while (1) {
        clearScreen();

        printf("\n====================================================\n");
        printf("                  CUSTOMER DASHBOARD                \n");
        printf("====================================================\n");
        printf("  [ User Profile ]\n");
        printf("  Name       : %s\n", currentUser.name);
        printf("  Account No : %d\n", currentUser.id);
        printf("  Phone      : %s\n", currentUser.phone);
        printf("\n");
        printf("  [ CURRENT BALANCE ]\n");
        printf("  Rs. %.2f\n", currentUser.balance);
        printf("====================================================\n");

        printf("  1. Deposit Money\n");
        printf("  2. Withdraw Money\n");
        printf("  3. Account Details (Print)\n");
        printf("  4. Transaction History (last %d)\n", HISTORY_SHOW);
        printf("  5. Logout\n");
        printf("----------------------------------------------------\n");
        printf("  Select Operation: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: // Deposit
                printf("\n  -- DEPOSIT SCREEN --");
                printf("\n  Enter Amount to Deposit: Rs.");
                scanf("%f", &amount);
                if (amount > 0) {
                    currentUser.balance += amount;
                    updateBalance(currentUser.id, currentUser.balance);
                    recordTransaction(&history, currentUser.id, 'D', amount, currentUser.balance);  // PUSH
                    printf("\n  [SUCCESS] Rs.%.2f added to your account.", amount);
                } else {
                    printf("\n  [ERROR] Invalid amount.");
                }
                break;

            case 2: // Withdraw
                printf("\n  -- WITHDRAWAL SCREEN --");
                printf("\n  Enter Amount to Withdraw: Rs.");
                scanf("%f", &amount);
                if (amount <= 0) {
                    printf("\n  [ERROR] Invalid amount.");
                } else if (amount > currentUser.balance) {
                    printf("\n  [ERROR] Insufficient funds (Rs.%.2f only).", currentUser.balance);
                } else {
                    currentUser.balance -= amount;
                    updateBalance(currentUser.id, currentUser.balance);
                    recordTransaction(&history, currentUser.id, 'W', amount, currentUser.balance);  // PUSH
                    printf("\n  [SUCCESS] Rs.%.2f withdrawn from account.", amount);
                    printf("\n  Remaining Balance: Rs.%.2f", currentUser.balance);
                }
                break;

            case 3: // Account details
                printf("\n  -- FULL DETAILS --");
                printf("\n  User: %s", currentUser.name);
                printf("\n  PAN Card: %s", currentUser.pan);
                printf("\n  Contact: %s", currentUser.phone);
                printf("\n  Balance: %.2f", currentUser.balance);
                printf("\n  Status: Active");
                break;

            case 4: // Transaction history (POP / PEEK demo)
                showHistory(&history, HISTORY_SHOW);
                break;

            case 5: // Logout
                printf("\n  Logging out...");
                clearStack(&history);        // free all stack nodes
                return;

            default:
                printf("\n  Invalid Choice.");
        }

        printf("\n\n  Press Enter to return to Dashboard...");
        getchar(); getchar();
    }
}

// UPDATE: find the record, step back one record, overwrite in place  O(n)
void updateBalance(int id, float newBalance) {
    FILE *fp = fopen("bank_database.dat", "rb+");
    struct User u;

    if (fp == NULL) return;

    while (fread(&u, sizeof(struct User), 1, fp)) {
        if (u.id == id) {
            u.balance = newBalance;
            fseek(fp, -(long)sizeof(struct User), SEEK_CUR);   // back one record
            fwrite(&u, sizeof(struct User), 1, fp);
            break;
        }
    }
    fclose(fp);
}

// ---------------------------------------------------------------
// Stack-based transaction history
// ---------------------------------------------------------------

// Read transactions.dat from oldest to newest and PUSH the ones for this user,
// so the newest transaction ends up on TOP of the stack.
void loadHistory(int id, struct Stack *s) {
    struct Transaction t;
    FILE *fp = fopen("transactions.dat", "rb");
    if (fp == NULL) return;                  // no history yet
    while (fread(&t, sizeof(struct Transaction), 1, fp)) {
        if (t.id == id) push(s, t);
    }
    fclose(fp);
}

// Save a transaction to file (permanent) AND push it on the live stack.
void recordTransaction(struct Stack *s, int id, char type, float amount, float balanceAfter) {
    struct Transaction t;
    t.id = id;
    t.type = type;
    t.amount = amount;
    t.balanceAfter = balanceAfter;

    FILE *fp = fopen("transactions.dat", "ab");
    if (fp != NULL) {
        fwrite(&t, sizeof(struct Transaction), 1, fp);
        fclose(fp);
    }
    push(s, t);
}

// Show the last n transactions, newest first (LIFO).
// Popping destroys the stack, so we move popped items to a temporary stack
// and then move them back, which restores the original order.
void showHistory(struct Stack *s, int n) {
    struct Stack temp;
    struct Transaction t;
    int shown = 0;

    initStack(&temp);

    printf("\n  -- LAST %d TRANSACTIONS (newest first) --\n", n);
    if (isEmpty(s)) {
        printf("  No transactions yet.");
        return;
    }

    while (shown < n && pop(s, &t)) {
        printf("  %d. %-8s Rs.%10.2f   Balance: Rs.%.2f\n",
               shown + 1,
               t.type == 'D' ? "Deposit" : "Withdraw",
               t.amount, t.balanceAfter);
        push(&temp, t);       // park it on the temp stack
        shown++;
    }

    while (pop(&temp, &t)) {  // put everything back in the original order
        push(s, t);
    }

    printf("  (Total transactions on record: %d)", s->size);
}

// Traversal of the whole database file  O(n)
void showUsers() {
    FILE *fp = fopen("bank_database.dat", "rb");
    struct User u;

    if (fp == NULL) {
        printf("\n [ERROR] No database found or unable to open file.\n");
        printf("\n Press Enter to return to Main Menu...");
        getchar(); getchar();
        return;
    }

    clearScreen();
    printHeader("SYSTEM DATABASE (ADMIN VIEW)");

    while (fread(&u, sizeof(struct User), 1, fp)) {
        printf(" ID       : %d\n", u.id);
        printf(" Name     : %s\n", u.name);
        printf(" Phone    : %s\n", u.phone);
        printf(" PAN      : %s\n", u.pan);
        printf(" Balance  : Rs. %.2f\n", u.balance);
        printf(" Password : %s\n", u.password);
        printf("----------------------------------------------------\n");
    }

    fclose(fp);

    printf("\n End of database. Press Enter to return to Main Menu...");
    getchar(); getchar();
}
