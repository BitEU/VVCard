#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <conio.h>
#include <direct.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

#pragma comment(lib, "user32.lib")

// Case-insensitive substring search
char* stristr(const char* haystack, const char* needle) {
    if (!needle || !*needle) {
        return (char*)haystack;
    }
    const char* h = haystack;
    const char* n = needle;
    while (*h) {
        const char* h_start = h;
        n = needle;
        while (*h && *n && (tolower((unsigned char)*h) == tolower((unsigned char)*n))) {
            h++;
            n++;
        }
        if (!*n) {
            return (char*)h_start; // Found
        }
        h = h_start + 1;
    }
    return NULL; // Not found
}

// --- UI & Style Constants ---
#define BOX_V "║"
#define BOX_H "═"
#define BOX_TL "╔"
#define BOX_TR "╗"
#define BOX_BL "╚"
#define BOX_BR "╝"
#define BOX_TJ "╦"
#define BOX_BJ "╩"
#define BOX_LJ "╠"
#define BOX_RJ "╣"
#define BOX_CROSS "╬"
#define SCROLL_TRACK '▒'
#define SCROLL_THUMB '█'

#define MAX_CONTACTS 1000
#define MAX_FIELD_LEN 512
#define MAX_LONG_FIELD_LEN 2048
#define MAX_EMAILS 5
#define MAX_PHONES 5
#define MAX_ADDRESSES 3

// Window dimensions
#define WINDOW_WIDTH 160
#define WINDOW_HEIGHT 48

#define FOREGROUND_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define FOREGROUND_GRAY (FOREGROUND_INTENSITY)
#define FOREGROUND_DARK_GRAY (0)
#define SELECTED_ITEM_COLOR (BACKGROUND_INTENSITY | FOREGROUND_BLUE)

// Contact field types
typedef enum {
    VIEW_LIST,
    VIEW_DETAIL,
    VIEW_EDIT,
    VIEW_NEW,
    VIEW_SEARCH
} ViewMode;

typedef struct {
    char type[32];
    char value[MAX_FIELD_LEN];
} TypedValue;

typedef struct {
    char pobox[MAX_FIELD_LEN];
    char extended[MAX_FIELD_LEN];
    char street[MAX_FIELD_LEN];
    char locality[MAX_FIELD_LEN];
    char region[MAX_FIELD_LEN];
    char postalcode[MAX_FIELD_LEN];
    char country[MAX_FIELD_LEN];
    char type[32];
    char label[MAX_FIELD_LEN];
} Address;

typedef struct {
    // Identification
    char uid[MAX_FIELD_LEN];
    char fn[MAX_FIELD_LEN];           // Full name (required)
    char family[MAX_FIELD_LEN];       // Family name
    char given[MAX_FIELD_LEN];        // Given name
    char additional[MAX_FIELD_LEN];   // Additional names
    char prefix[MAX_FIELD_LEN];       // Honorific prefixes
    char suffix[MAX_FIELD_LEN];       // Honorific suffixes
    char nickname[MAX_FIELD_LEN];
    
    // Contact info
    TypedValue emails[MAX_EMAILS];
    int email_count;
    TypedValue phones[MAX_PHONES];
    int phone_count;
    Address addresses[MAX_ADDRESSES];
    int address_count;
    
    // Personal info
    char bday[MAX_FIELD_LEN];         // Birthday
    char anniversary[MAX_FIELD_LEN];
    char gender[MAX_FIELD_LEN];
    char lang[MAX_FIELD_LEN];         // Preferred language
    char tz[MAX_FIELD_LEN];           // Timezone
    
    // Professional info
    char org[MAX_FIELD_LEN];          // Organization
    char title[MAX_FIELD_LEN];        // Job title
    char role[MAX_FIELD_LEN];         // Role/occupation
    
    // Online presence
    char url[MAX_FIELD_LEN];          // Website
    char twitter[MAX_FIELD_LEN];
    char linkedin[MAX_FIELD_LEN];
    char impp[MAX_FIELD_LEN];         // Instant messaging
    
    // Notes and metadata
    char note[MAX_LONG_FIELD_LEN];    // Notes
    char categories[MAX_FIELD_LEN];   // Categories/tags
    char prodid[MAX_FIELD_LEN];       // Product ID (this software)
    char rev[MAX_FIELD_LEN];          // Last revision timestamp
    char created[MAX_FIELD_LEN];      // Creation timestamp
    
    // Additional fields
    char geo[MAX_FIELD_LEN];          // Geographic coordinates
    char kind[MAX_FIELD_LEN];         // Kind of object (individual/group/org/location)
    char birthplace[MAX_FIELD_LEN];   // Birth place
    char birthplace_geo[MAX_FIELD_LEN]; // Birth place coordinates
    char deathplace[MAX_FIELD_LEN];   // Death place
    char deathplace_geo[MAX_FIELD_LEN]; // Death place coordinates
    char deathdate[MAX_FIELD_LEN];    // Death date
    char related[MAX_FIELD_LEN];      // Related person
    char expertise[MAX_FIELD_LEN];    // Areas of expertise
    char hobby[MAX_FIELD_LEN];        // Hobbies
    char interest[MAX_FIELD_LEN];     // Interests
} Contact;

typedef struct {
    Contact contacts[MAX_CONTACTS];
    int count;
    int selected;
    int scroll_offset;
    ViewMode mode;
    int edit_field;
    char search_term[MAX_FIELD_LEN];
    Contact* current_contact;
    Contact edit_buffer;
} ContactList;

typedef struct {
    CONSOLE_SCREEN_BUFFER_INFO original_screen_info;
    CONSOLE_CURSOR_INFO original_cursor_info;
    COORD original_window_size;
    SMALL_RECT original_window_rect;
    WORD original_attributes;
} OriginalConsoleSettings;

// Function prototypes
void generate_uid(char* uid);
void get_current_timestamp(char* timestamp);
void update_revision(Contact* contact);
void save_contacts(ContactList* list);
void load_contacts(ContactList* list);

// Console functions
void gotoxy(int x, int y) {
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void clear_screen() {
    system("cls");
}

void set_color(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void store_original_settings(OriginalConsoleSettings* settings) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &settings->original_screen_info);
    GetConsoleCursorInfo(hConsole, &settings->original_cursor_info);
    settings->original_window_size = settings->original_screen_info.dwSize;
    settings->original_window_rect = settings->original_screen_info.srWindow;
    settings->original_attributes = settings->original_screen_info.wAttributes;
}

void restore_original_settings(OriginalConsoleSettings* settings) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleScreenBufferSize(hConsole, settings->original_window_size);
    SetConsoleWindowInfo(hConsole, TRUE, &settings->original_window_rect);
    SetConsoleCursorInfo(hConsole, &settings->original_cursor_info);
    SetConsoleTextAttribute(hConsole, settings->original_attributes);
    system("cls");
}

void setup_console_size() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    HWND hwnd = GetConsoleWindow();
    if (hwnd == NULL) {
        printf("Error: Could not get console window handle.\n");
        printf("This program may need to be run in a standard Windows console.\n");
        exit(1);
    }

    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~WS_SIZEBOX;
    style &= ~WS_MAXIMIZEBOX;
    SetWindowLong(hwnd, GWL_STYLE, style);
    
    COORD bufferSize = { WINDOW_WIDTH, WINDOW_HEIGHT + 10 };
    SetConsoleScreenBufferSize(hConsole, bufferSize);
    
    SMALL_RECT windowRect = { 0, 0, WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1 };
    SetConsoleWindowInfo(hConsole, TRUE, &windowRect);
}

// UI Drawing functions
void draw_title_bar(const char* title) {
    set_color(BACKGROUND_BLUE | FOREGROUND_WHITE);
    for (int i = 0; i < WINDOW_WIDTH; i++) {
        gotoxy(i, 0);
        printf(" ");
    }
    gotoxy((WINDOW_WIDTH - strlen(title)) / 2, 0);
    printf("%s", title);
    set_color(FOREGROUND_WHITE);
}

void draw_status_bar(const char* status) {
    set_color(BACKGROUND_BLUE | FOREGROUND_WHITE);
    for (int i = 0; i < WINDOW_WIDTH; i++) {
        gotoxy(i, WINDOW_HEIGHT - 1);
        printf(" ");
    }
    gotoxy(3, WINDOW_HEIGHT - 1);
    printf("%s", status);
    set_color(FOREGROUND_WHITE);
}

void draw_borders() {
    set_color(FOREGROUND_GRAY);
    
    // Main window border
    gotoxy(0, 1); printf(BOX_TL);
    for (int i = 1; i < WINDOW_WIDTH - 1; i++) printf(BOX_H);
    printf(BOX_TR);

    for (int i = 2; i < WINDOW_HEIGHT - 2; i++) {
        gotoxy(0, i); printf(BOX_V);
        gotoxy(WINDOW_WIDTH - 1, i); printf(BOX_V);
    }

    gotoxy(0, WINDOW_HEIGHT - 2); printf(BOX_BL);
    for (int i = 1; i < WINDOW_WIDTH - 1; i++) printf(BOX_H);
    printf(BOX_BR);
}

void draw_scrollbar(int total_items, int selected, int scroll_offset, int list_height, int x_pos) {
    if (total_items <= list_height) return;

    set_color(FOREGROUND_DARK_GRAY | BACKGROUND_INTENSITY);
    for (int i = 0; i < list_height; i++) {
        gotoxy(x_pos, 3 + i);
        printf("%c", SCROLL_TRACK);
    }

    float thumb_pos_f = (float)selected / (total_items - 1);
    int thumb_pos = (int)(thumb_pos_f * (list_height - 1));

    set_color(FOREGROUND_WHITE | BACKGROUND_BLUE);
    gotoxy(x_pos, 3 + thumb_pos);
    printf("%c", SCROLL_THUMB);
}

void draw_contact_list(ContactList* list) {
    int list_height = WINDOW_HEIGHT - 6;
    int visible_items = list_height;

    // Draw column headers
    gotoxy(2, 2);
    set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
    printf("%-40s %-30s %-25s %-20s", "Name", "Organization", "Email", "Phone");

    // Draw separator line
    gotoxy(1, 3);
    set_color(FOREGROUND_GRAY);
    for (int i = 1; i < WINDOW_WIDTH - 1; i++) printf("─");

    // Draw contacts
    for (int i = 0; i < visible_items; i++) {
        gotoxy(2, 4 + i);
        int contact_index = i + list->scroll_offset;
        if (contact_index < list->count) {
            Contact* c = &list->contacts[contact_index];
            
            if (contact_index == list->selected) {
                set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
                printf(">");
            } else {
                set_color(FOREGROUND_GRAY);
                printf(" ");
            }
            
            char email[MAX_FIELD_LEN] = "";
            if (c->email_count > 0) strncpy(email, c->emails[0].value, 25);
            
            char phone[MAX_FIELD_LEN] = "";
            if (c->phone_count > 0) strncpy(phone, c->phones[0].value, 20);
            
            printf("%-39.39s %-30.30s %-25.25s %-20.20s", 
                   c->fn, c->org, email, phone);
        } else {
            printf("%-155s", " ");
        }
    }
    
    draw_scrollbar(list->count, list->selected, list->scroll_offset, list_height, WINDOW_WIDTH - 2);
}

void draw_contact_detail(Contact* contact) {
    draw_title_bar("Contact Details");
    set_color(FOREGROUND_WHITE);
    draw_borders();
    
    int y = 3;
    int x_label = 3;
    int x_value = 25;
    int x_label2 = 80;
    int x_value2 = 102;
    
    set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
    gotoxy(x_label, y); printf("Full Name:");
    set_color(FOREGROUND_GREEN);
    gotoxy(x_value, y); printf("%s", contact->fn);
    
    y += 2;
    
    // Name components
    if (strlen(contact->prefix) > 0) {
        set_color(FOREGROUND_WHITE);
        gotoxy(x_label, y); printf("Prefix:");
        set_color(FOREGROUND_GRAY);
        gotoxy(x_value, y); printf("%s", contact->prefix);
        y++;
    }
    
    if (strlen(contact->given) > 0) {
        set_color(FOREGROUND_WHITE);
        gotoxy(x_label, y); printf("Given Name:");
        set_color(FOREGROUND_GRAY);
        gotoxy(x_value, y); printf("%s", contact->given);
        
        if (strlen(contact->family) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label2, y); printf("Family Name:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value2, y); printf("%s", contact->family);
        }
        y++;
    }
    
    if (strlen(contact->nickname) > 0) {
        set_color(FOREGROUND_WHITE);
        gotoxy(x_label, y); printf("Nickname:");
        set_color(FOREGROUND_GRAY);
        gotoxy(x_value, y); printf("%s", contact->nickname);
        y++;
    }
    
    y++;
    
    // Contact Information
    set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
    gotoxy(x_label, y); printf("── Contact Information ──");
    y += 2;
    
    // Emails
    for (int i = 0; i < contact->email_count; i++) {
        set_color(FOREGROUND_WHITE);
        gotoxy(x_label, y); printf("Email %s:", contact->emails[i].type);
        set_color(FOREGROUND_GRAY);
        gotoxy(x_value, y); printf("%s", contact->emails[i].value);
        y++;
    }
    
    // Phones
    for (int i = 0; i < contact->phone_count; i++) {
        set_color(FOREGROUND_WHITE);
        gotoxy(x_label, y); printf("Phone %s:", contact->phones[i].type);
        set_color(FOREGROUND_GRAY);
        gotoxy(x_value, y); printf("%s", contact->phones[i].value);
        y++;
    }
    
    y++;
    
    // Professional Information
    if (strlen(contact->org) > 0 || strlen(contact->title) > 0) {
        set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
        gotoxy(x_label, y); printf("── Professional Information ──");
        y += 2;
        
        if (strlen(contact->org) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label, y); printf("Organization:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value, y); printf("%s", contact->org);
            y++;
        }
        
        if (strlen(contact->title) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label, y); printf("Title:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value, y); printf("%s", contact->title);
            y++;
        }
        
        if (strlen(contact->role) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label, y); printf("Role:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value, y); printf("%s", contact->role);
            y++;
        }
    }
    
    // Personal Information
    y = 3;  // Reset to top right column
    
    if (strlen(contact->bday) > 0 || strlen(contact->anniversary) > 0 || strlen(contact->birthplace) > 0 || strlen(contact->deathdate) > 0 || strlen(contact->deathplace) > 0) {
        set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
        gotoxy(x_label2, y); printf("── Personal Information ──");
        y += 2;
        
        if (strlen(contact->bday) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label2, y); printf("Birthday:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value2, y); printf("%s", contact->bday);
            y++;
        }
        
        if (strlen(contact->birthplace) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label2, y); printf("Birth Place:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value2, y); printf("%s", contact->birthplace);
            y++;
        }
        
        if (strlen(contact->anniversary) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label2, y); printf("Anniversary:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value2, y); printf("%s", contact->anniversary);
            y++;
        }
        
        if (strlen(contact->deathdate) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label2, y); printf("Death Date:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value2, y); printf("%s", contact->deathdate);
            y++;
        }
        
        if (strlen(contact->deathplace) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label2, y); printf("Death Place:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value2, y); printf("%s", contact->deathplace);
            y++;
        }
    }
    
    // Addresses
    if (contact->address_count > 0) {
        y++;
        set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
        gotoxy(x_label2, y); printf("── Addresses ──");
        y += 2;
        
        for (int i = 0; i < contact->address_count; i++) {
            Address* addr = &contact->addresses[i];
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label2, y); printf("%s Address:", addr->type);
            y++;
            set_color(FOREGROUND_GRAY);
            if (strlen(addr->street) > 0) {
                gotoxy(x_value2, y); printf("%s", addr->street);
                y++;
            }
            if (strlen(addr->locality) > 0 || strlen(addr->region) > 0) {
                gotoxy(x_value2, y); 
                printf("%s%s%s", addr->locality, 
                       (strlen(addr->locality) > 0 && strlen(addr->region) > 0) ? ", " : "",
                       addr->region);
                y++;
            }
            if (strlen(addr->postalcode) > 0 || strlen(addr->country) > 0) {
                gotoxy(x_value2, y);
                printf("%s %s", addr->postalcode, addr->country);
                y++;
            }
            y++;
        }
    }
    
    // Online presence
    if (strlen(contact->url) > 0 || strlen(contact->linkedin) > 0 || strlen(contact->impp) > 0) {
        y++;
        set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
        gotoxy(x_label2, y); printf("── Online Presence ──");
        y += 2;
        
        if (strlen(contact->url) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label2, y); printf("Website:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value2, y); printf("%s", contact->url);
            y++;
        }
        
        if (strlen(contact->impp) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label2, y); printf("Instant Msg:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value2, y); printf("%s", contact->impp);
            y++;
        }
    }
    
    // Interests & Skills
    if (strlen(contact->related) > 0 || strlen(contact->expertise) > 0 || strlen(contact->hobby) > 0 || strlen(contact->interest) > 0) {
        y++;
        set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
        gotoxy(x_label2, y); printf("── Interests & Skills ──");
        y += 2;
        
        if (strlen(contact->related) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label2, y); printf("Related:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value2, y); printf("%s", contact->related);
            y++;
        }
        
        if (strlen(contact->expertise) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label2, y); printf("Expertise:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value2, y); printf("%s", contact->expertise);
            y++;
        }
        
        if (strlen(contact->hobby) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label2, y); printf("Hobby:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value2, y); printf("%s", contact->hobby);
            y++;
        }
        
        if (strlen(contact->interest) > 0) {
            set_color(FOREGROUND_WHITE);
            gotoxy(x_label2, y); printf("Interest:");
            set_color(FOREGROUND_GRAY);
            gotoxy(x_value2, y); printf("%s", contact->interest);
            y++;
        }
    }
    
    // Notes
    if (strlen(contact->note) > 0) {
        y = WINDOW_HEIGHT - 10;
        set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
        gotoxy(x_label, y); printf("── Notes ──");
        y++;
        set_color(FOREGROUND_GRAY);
        
        // Word wrap notes
        char* note_copy = _strdup(contact->note);
        char* word = strtok(note_copy, " ");
        int current_x = x_label;
        
        while(word != NULL && y < WINDOW_HEIGHT - 3) {
            if (current_x + strlen(word) > WINDOW_WIDTH - 5) {
                y++;
                current_x = x_label;
            }
            gotoxy(current_x, y);
            printf("%s ", word);
            current_x += strlen(word) + 1;
            word = strtok(NULL, " ");
        }
        free(note_copy);
    }
    
    draw_status_bar("Press E to Edit, D to Delete, ESC to go back");
}

void draw_edit_form(Contact* contact, int selected_field) {
    draw_title_bar("Edit Contact");
    draw_borders();
    
    int y = 3;
    int x_label = 3;
    int x_input = 25;
    int field_index = 0;
    
    // Helper macro for field drawing
    #define DRAW_FIELD(label, value, max_len) do { \
        set_color((field_index == selected_field) ? FOREGROUND_WHITE | FOREGROUND_INTENSITY : FOREGROUND_WHITE); \
        gotoxy(x_label, y); printf("%s:", label); \
        set_color((field_index == selected_field) ? FOREGROUND_GREEN | FOREGROUND_INTENSITY : FOREGROUND_GRAY); \
        gotoxy(x_input, y); \
        if (field_index == selected_field) { \
            printf("[%-*s]", max_len, value); \
        } else { \
            /* Print underscores for empty space */ \
            char temp[256]; \
            snprintf(temp, sizeof(temp), "%-*s", max_len, value); \
            for (int i = 0; i < max_len; i++) { \
                if (i < strlen(value)) { \
                    printf("%c", temp[i]); \
                } else { \
                    printf("_"); \
                } \
            } \
            printf("  "); /* Two spaces to clear any remaining brackets */ \
        } \
        y++; \
        field_index++; \
    } while(0)
    
    set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
    gotoxy(x_label, y); printf("── Name Information ──");
    y += 2;
    
    DRAW_FIELD("Full Name", contact->fn, 50);
    DRAW_FIELD("Prefix", contact->prefix, 20);
    DRAW_FIELD("Given Name", contact->given, 30);
    DRAW_FIELD("Family Name", contact->family, 30);
    DRAW_FIELD("Nickname", contact->nickname, 30);
    
    y++;
    set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
    gotoxy(x_label, y); printf("── Contact Information ──");
    y += 2;
    
    // Email fields
    for (int i = 0; i < MAX_EMAILS; i++) {
        char label[32];
        sprintf(label, "Email %d", i + 1);
        DRAW_FIELD(label, i < contact->email_count ? contact->emails[i].value : "", 40);
    }
    
    // Phone fields
    for (int i = 0; i < MAX_PHONES; i++) {
        char label[32];
        sprintf(label, "Phone %d", i + 1);
        DRAW_FIELD(label, i < contact->phone_count ? contact->phones[i].value : "", 30);
    }
    
    y++;
    set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
    gotoxy(x_label, y); printf("── Professional Information ──");
    y += 2;
    
    DRAW_FIELD("Organization", contact->org, 50);
    DRAW_FIELD("Title", contact->title, 40);
    DRAW_FIELD("Role", contact->role, 40);
    
    y++;
    set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
    gotoxy(x_label, y); printf("── Personal Information ──");
    y += 2;
    
    DRAW_FIELD("Birthday", contact->bday, 20);
    DRAW_FIELD("Anniversary", contact->anniversary, 20);
    DRAW_FIELD("Language", contact->lang, 20);
    DRAW_FIELD("Timezone", contact->tz, 30);
    
    // Second column
    y = 3;
    x_label = 80;
    x_input = 102;
    
    set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
    gotoxy(x_label, y); printf("── Address (Home) ──");
    y += 2;
    
    if (contact->address_count == 0) {
        contact->address_count = 1;
        strcpy(contact->addresses[0].type, "HOME");
    }
    
    DRAW_FIELD("Street", contact->addresses[0].street, 40);
    DRAW_FIELD("City", contact->addresses[0].locality, 30);
    DRAW_FIELD("State/Region", contact->addresses[0].region, 20);
    DRAW_FIELD("Postal Code", contact->addresses[0].postalcode, 15);
    DRAW_FIELD("Country", contact->addresses[0].country, 30);
    
    y++;
    set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
    gotoxy(x_label, y); printf("── Online Presence ──");
    y += 2;
    
    DRAW_FIELD("Website", contact->url, 50);
    DRAW_FIELD("LinkedIn", contact->linkedin, 40);
    DRAW_FIELD("Twitter", contact->twitter, 30);
    DRAW_FIELD("Instant Msg", contact->impp, 40);
    
    y++;
    set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
    gotoxy(x_label, y); printf("── Life Events ──");
    y += 2;
    
    DRAW_FIELD("Birth Place", contact->birthplace, 40);
    DRAW_FIELD("Birth Place Geo", contact->birthplace_geo, 30);
    DRAW_FIELD("Death Date", contact->deathdate, 20);
    DRAW_FIELD("Death Place", contact->deathplace, 40);
    DRAW_FIELD("Death Place Geo", contact->deathplace_geo, 30);
    
    y++;
    set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
    gotoxy(x_label, y); printf("── Interests & Skills ──");
    y += 2;
    
    DRAW_FIELD("Related", contact->related, 40);
    DRAW_FIELD("Expertise", contact->expertise, 40);
    DRAW_FIELD("Hobby", contact->hobby, 40);
    DRAW_FIELD("Interest", contact->interest, 40);
    
    y++;
    set_color(FOREGROUND_WHITE | FOREGROUND_INTENSITY);
    gotoxy(x_label, y); printf("── Additional ──");
    y += 2;
    
    DRAW_FIELD("Categories", contact->categories, 40);
    DRAW_FIELD("Notes", contact->note, 50);
    
    #undef DRAW_FIELD
    
    draw_status_bar("Use ↑/↓ to navigate, Enter to edit field, S to Save, ESC to Cancel");
}

// VCARD functions
void generate_uid(char* uid) {
    sprintf(uid, "urn:uuid:%08x-%04x-%04x-%04x-%012x",
            rand(), rand() & 0xFFFF, rand() & 0xFFFF, 
            rand() & 0xFFFF, rand());
}

void get_current_timestamp(char* timestamp) {
    time_t now = time(NULL);
    struct tm* tm_info = gmtime(&now);
    strftime(timestamp, MAX_FIELD_LEN, "%Y%m%dT%H%M%SZ", tm_info);
}

void update_revision(Contact* contact) {
    get_current_timestamp(contact->rev);
}

void escape_vcard_value(const char* input, char* output) {
    const char* p = input;
    char* q = output;
    
    while (*p) {
        switch (*p) {
            case '\\': *q++ = '\\'; *q++ = '\\'; break;
            case ',': *q++ = '\\'; *q++ = ','; break;
            case ';': *q++ = '\\'; *q++ = ';'; break;
            case '\n': *q++ = '\\'; *q++ = 'n'; break;
            case '\r': break; // Skip carriage returns
            default: *q++ = *p;
        }
        p++;
    }
    *q = '\0';
}

void unescape_vcard_value(const char* input, char* output) {
    const char* p = input;
    char* q = output;
    
    while (*p) {
        if (*p == '\\' && *(p + 1)) {
            p++;
            switch (*p) {
                case '\\': *q++ = '\\'; break;
                case ',': *q++ = ','; break;
                case ';': *q++ = ';'; break;
                case 'n': *q++ = '\n'; break;
                case 'N': *q++ = '\n'; break;
                default: *q++ = *p;
            }
            p++;
        } else {
            *q++ = *p++;
        }
    }
    *q = '\0';
}

void write_vcard_contact(FILE* file, Contact* contact) {
    char escaped[MAX_LONG_FIELD_LEN];
    
    fprintf(file, "BEGIN:VCARD\n");
    fprintf(file, "VERSION:4.0\n");
    
    // UID (required)
    if (strlen(contact->uid) == 0) {
        generate_uid(contact->uid);
    }
    fprintf(file, "UID:%s\n", contact->uid);
    
    // REV (revision timestamp)
    fprintf(file, "REV:%s\n", contact->rev);
    
    // PRODID
    fprintf(file, "PRODID:-//VCard Contact Manager//EN\n");
    
    // FN (required)
    escape_vcard_value(contact->fn, escaped);
    fprintf(file, "FN:%s\n", escaped);
    
    // N (structured name)
    if (strlen(contact->family) > 0 || strlen(contact->given) > 0) {
        char family_esc[MAX_FIELD_LEN], given_esc[MAX_FIELD_LEN];
        char additional_esc[MAX_FIELD_LEN], prefix_esc[MAX_FIELD_LEN], suffix_esc[MAX_FIELD_LEN];
        
        escape_vcard_value(contact->family, family_esc);
        escape_vcard_value(contact->given, given_esc);
        escape_vcard_value(contact->additional, additional_esc);
        escape_vcard_value(contact->prefix, prefix_esc);
        escape_vcard_value(contact->suffix, suffix_esc);
        
        fprintf(file, "N:%s;%s;%s;%s;%s\n", 
                family_esc, given_esc, additional_esc, prefix_esc, suffix_esc);
    }
    
    // NICKNAME
    if (strlen(contact->nickname) > 0) {
        escape_vcard_value(contact->nickname, escaped);
        fprintf(file, "NICKNAME:%s\n", escaped);
    }
    
    // EMAIL
    for (int i = 0; i < contact->email_count; i++) {
        escape_vcard_value(contact->emails[i].value, escaped);
        fprintf(file, "EMAIL;TYPE=%s:%s\n", contact->emails[i].type, escaped);
    }
    
    // TEL
    for (int i = 0; i < contact->phone_count; i++) {
        escape_vcard_value(contact->phones[i].value, escaped);
        fprintf(file, "TEL;TYPE=%s:%s\n", contact->phones[i].type, escaped);
    }
    
    // ADR
    for (int i = 0; i < contact->address_count; i++) {
        Address* addr = &contact->addresses[i];
        char pobox_esc[MAX_FIELD_LEN], ext_esc[MAX_FIELD_LEN], street_esc[MAX_FIELD_LEN];
        char locality_esc[MAX_FIELD_LEN], region_esc[MAX_FIELD_LEN];
        char postal_esc[MAX_FIELD_LEN], country_esc[MAX_FIELD_LEN];
        
        escape_vcard_value(addr->pobox, pobox_esc);
        escape_vcard_value(addr->extended, ext_esc);
        escape_vcard_value(addr->street, street_esc);
        escape_vcard_value(addr->locality, locality_esc);
        escape_vcard_value(addr->region, region_esc);
        escape_vcard_value(addr->postalcode, postal_esc);
        escape_vcard_value(addr->country, country_esc);
        
        fprintf(file, "ADR;TYPE=%s:%s;%s;%s;%s;%s;%s;%s\n",
                addr->type, pobox_esc, ext_esc, street_esc,
                locality_esc, region_esc, postal_esc, country_esc);
    }
    
    // ORG
    if (strlen(contact->org) > 0) {
        escape_vcard_value(contact->org, escaped);
        fprintf(file, "ORG:%s\n", escaped);
    }
    
    // TITLE
    if (strlen(contact->title) > 0) {
        escape_vcard_value(contact->title, escaped);
        fprintf(file, "TITLE:%s\n", escaped);
    }
    
    // ROLE
    if (strlen(contact->role) > 0) {
        escape_vcard_value(contact->role, escaped);
        fprintf(file, "ROLE:%s\n", escaped);
    }
    
    // BDAY
    if (strlen(contact->bday) > 0) {
        fprintf(file, "BDAY:%s\n", contact->bday);
    }
    
    // ANNIVERSARY
    if (strlen(contact->anniversary) > 0) {
        fprintf(file, "ANNIVERSARY:%s\n", contact->anniversary);
    }
    
    // GENDER
    if (strlen(contact->gender) > 0) {
        fprintf(file, "GENDER:%s\n", contact->gender);
    }
    
    // LANG
    if (strlen(contact->lang) > 0) {
        fprintf(file, "LANG:%s\n", contact->lang);
    }
    
    // TZ
    if (strlen(contact->tz) > 0) {
        fprintf(file, "TZ:%s\n", contact->tz);
    }
    
    // URL
    if (strlen(contact->url) > 0) {
        escape_vcard_value(contact->url, escaped);
        fprintf(file, "URL:%s\n", escaped);
    }
    
    // Social media URLs
    if (strlen(contact->linkedin) > 0) {
        escape_vcard_value(contact->linkedin, escaped);
        fprintf(file, "URL;TYPE=linkedin:%s\n", escaped);
    }
    
    if (strlen(contact->twitter) > 0) {
        escape_vcard_value(contact->twitter, escaped);
        fprintf(file, "URL;TYPE=twitter:%s\n", escaped);
    }
    
    // IMPP (Instant messaging)
    if (strlen(contact->impp) > 0) {
        escape_vcard_value(contact->impp, escaped);
        fprintf(file, "IMPP:%s\n", escaped);
    }
    
    // GEO (Geographic coordinates)
    if (strlen(contact->geo) > 0) {
        fprintf(file, "GEO:%s\n", contact->geo);
    }
    
    // BIRTHPLACE with GEO
    if (strlen(contact->birthplace) > 0) {
        escape_vcard_value(contact->birthplace, escaped);
        fprintf(file, "BIRTHPLACE:%s\n", escaped);
        
        if (strlen(contact->birthplace_geo) > 0) {
            fprintf(file, "X-BIRTHPLACE-GEO:%s\n", contact->birthplace_geo);
        }
    }
    
    // DEATHPLACE with GEO
    if (strlen(contact->deathplace) > 0) {
        escape_vcard_value(contact->deathplace, escaped);
        fprintf(file, "DEATHPLACE:%s\n", escaped);
        
        if (strlen(contact->deathplace_geo) > 0) {
            fprintf(file, "X-DEATHPLACE-GEO:%s\n", contact->deathplace_geo);
        }
    }
    
    // DEATHDATE
    if (strlen(contact->deathdate) > 0) {
        fprintf(file, "DEATHDATE:%s\n", contact->deathdate);
    }
    
    // RELATED
    if (strlen(contact->related) > 0) {
        escape_vcard_value(contact->related, escaped);
        fprintf(file, "RELATED:%s\n", escaped);
    }
    
    // EXPERTISE
    if (strlen(contact->expertise) > 0) {
        escape_vcard_value(contact->expertise, escaped);
        fprintf(file, "EXPERTISE:%s\n", escaped);
    }
    
    // HOBBY
    if (strlen(contact->hobby) > 0) {
        escape_vcard_value(contact->hobby, escaped);
        fprintf(file, "HOBBY:%s\n", escaped);
    }
    
    // INTEREST
    if (strlen(contact->interest) > 0) {
        escape_vcard_value(contact->interest, escaped);
        fprintf(file, "INTEREST:%s\n", escaped);
    }
    
    // CATEGORIES
    if (strlen(contact->categories) > 0) {
        escape_vcard_value(contact->categories, escaped);
        fprintf(file, "CATEGORIES:%s\n", escaped);
    }
    
    // NOTE
    if (strlen(contact->note) > 0) {
        escape_vcard_value(contact->note, escaped);
        fprintf(file, "NOTE:%s\n", escaped);
    }
    
    // KIND
    if (strlen(contact->kind) > 0) {
        fprintf(file, "KIND:%s\n", contact->kind);
    } else {
        fprintf(file, "KIND:individual\n");
    }
    
    fprintf(file, "END:VCARD\n\n");
}

int parse_vcard_line(const char* line, char* property, char* params, char* value) {
    const char* colon = strchr(line, ':');
    if (!colon) return 0;
    
    const char* semicolon = strchr(line, ';');
    
    if (semicolon && semicolon < colon) {
        // Has parameters
        strncpy(property, line, semicolon - line);
        property[semicolon - line] = '\0';
        strncpy(params, semicolon + 1, colon - semicolon - 1);
        params[colon - semicolon - 1] = '\0';
    } else {
        // No parameters
        strncpy(property, line, colon - line);
        property[colon - line] = '\0';
        params[0] = '\0';
    }
    
    strcpy(value, colon + 1);
    // Remove trailing newline
    char* newline = strchr(value, '\n');
    if (newline) *newline = '\0';
    newline = strchr(value, '\r');
    if (newline) *newline = '\0';
    
    return 1;
}

void parse_structured_name(const char* value, Contact* contact) {
    char temp[MAX_FIELD_LEN];
    char* fields[5] = {contact->family, contact->given, contact->additional, 
                       contact->prefix, contact->suffix};
    int field_index = 0;
    const char* p = value;
    char* q = temp;
    
    while (*p && field_index < 5) {
        if (*p == ';') {
            *q = '\0';
            unescape_vcard_value(temp, fields[field_index]);
            field_index++;
            q = temp;
            p++;
        } else if (*p == '\\' && *(p + 1) == ';') {
            *q++ = ';';
            p += 2;
        } else {
            *q++ = *p++;
        }
    }
    if (field_index < 5) {
        *q = '\0';
        unescape_vcard_value(temp, fields[field_index]);
    }
}

void parse_address(const char* value, Address* addr) {
    char temp[MAX_FIELD_LEN];
    char* fields[7] = {addr->pobox, addr->extended, addr->street, 
                       addr->locality, addr->region, addr->postalcode, addr->country};
    int field_index = 0;
    const char* p = value;
    char* q = temp;
    
    while (*p && field_index < 7) {
        if (*p == ';') {
            *q = '\0';
            unescape_vcard_value(temp, fields[field_index]);
            field_index++;
            q = temp;
            p++;
        } else if (*p == '\\' && *(p + 1) == ';') {
            *q++ = ';';
            p += 2;
        } else {
            *q++ = *p++;
        }
    }
    if (field_index < 7) {
        *q = '\0';
        unescape_vcard_value(temp, fields[field_index]);
    }
}

void read_vcard_contact(FILE* file, Contact* contact) {
    char line[MAX_LONG_FIELD_LEN];
    char property[64], params[256], value[MAX_LONG_FIELD_LEN];
    
    memset(contact, 0, sizeof(Contact));
    strcpy(contact->kind, "individual");
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "END:VCARD", 9) == 0) break;
        
        if (!parse_vcard_line(line, property, params, value)) continue;
        
        if (strcmp(property, "UID") == 0) {
            strcpy(contact->uid, value);
        } else if (strcmp(property, "FN") == 0) {
            unescape_vcard_value(value, contact->fn);
        } else if (strcmp(property, "N") == 0) {
            parse_structured_name(value, contact);
        } else if (strcmp(property, "NICKNAME") == 0) {
            unescape_vcard_value(value, contact->nickname);
        } else if (strcmp(property, "EMAIL") == 0) {
            if (contact->email_count < MAX_EMAILS) {
                unescape_vcard_value(value, contact->emails[contact->email_count].value);
                // Extract TYPE parameter
                char* type_start = strstr(params, "TYPE=");
                if (type_start) {
                    sscanf(type_start + 5, "%[^;]", contact->emails[contact->email_count].type);
                } else {
                    strcpy(contact->emails[contact->email_count].type, "WORK");
                }
                contact->email_count++;
            }
        } else if (strcmp(property, "TEL") == 0) {
            if (contact->phone_count < MAX_PHONES) {
                unescape_vcard_value(value, contact->phones[contact->phone_count].value);
                char* type_start = strstr(params, "TYPE=");
                if (type_start) {
                    sscanf(type_start + 5, "%[^;]", contact->phones[contact->phone_count].type);
                } else {
                    strcpy(contact->phones[contact->phone_count].type, "WORK");
                }
                contact->phone_count++;
            }
        } else if (strcmp(property, "ADR") == 0) {
            if (contact->address_count < MAX_ADDRESSES) {
                parse_address(value, &contact->addresses[contact->address_count]);
                char* type_start = strstr(params, "TYPE=");
                if (type_start) {
                    sscanf(type_start + 5, "%[^;]", contact->addresses[contact->address_count].type);
                } else {
                    strcpy(contact->addresses[contact->address_count].type, "HOME");
                }
                contact->address_count++;
            }
        } else if (strcmp(property, "ORG") == 0) {
            unescape_vcard_value(value, contact->org);
        } else if (strcmp(property, "TITLE") == 0) {
            unescape_vcard_value(value, contact->title);
        } else if (strcmp(property, "ROLE") == 0) {
            unescape_vcard_value(value, contact->role);
        } else if (strcmp(property, "BDAY") == 0) {
            strcpy(contact->bday, value);
        } else if (strcmp(property, "ANNIVERSARY") == 0) {
            strcpy(contact->anniversary, value);
        } else if (strcmp(property, "GENDER") == 0) {
            strcpy(contact->gender, value);
        } else if (strcmp(property, "LANG") == 0) {
            strcpy(contact->lang, value);
        } else if (strcmp(property, "TZ") == 0) {
            strcpy(contact->tz, value);
        } else if (strcmp(property, "URL") == 0) {
            if (strstr(params, "TYPE=linkedin")) {
                unescape_vcard_value(value, contact->linkedin);
            } else if (strstr(params, "TYPE=twitter")) {
                unescape_vcard_value(value, contact->twitter);
            } else {
                unescape_vcard_value(value, contact->url);
            }
        } else if (strcmp(property, "CATEGORIES") == 0) {
            unescape_vcard_value(value, contact->categories);
        } else if (strcmp(property, "NOTE") == 0) {
            unescape_vcard_value(value, contact->note);
        } else if (strcmp(property, "REV") == 0) {
            strcpy(contact->rev, value);
        } else if (strcmp(property, "KIND") == 0) {
            strcpy(contact->kind, value);
        } else if (strcmp(property, "IMPP") == 0) {
            unescape_vcard_value(value, contact->impp);
        } else if (strcmp(property, "GEO") == 0) {
            strcpy(contact->geo, value);
        } else if (strcmp(property, "BIRTHPLACE") == 0) {
            unescape_vcard_value(value, contact->birthplace);
        } else if (strcmp(property, "X-BIRTHPLACE-GEO") == 0) {
            strcpy(contact->birthplace_geo, value);
        } else if (strcmp(property, "DEATHPLACE") == 0) {
            unescape_vcard_value(value, contact->deathplace);
        } else if (strcmp(property, "X-DEATHPLACE-GEO") == 0) {
            strcpy(contact->deathplace_geo, value);
        } else if (strcmp(property, "DEATHDATE") == 0) {
            strcpy(contact->deathdate, value);
        } else if (strcmp(property, "RELATED") == 0) {
            unescape_vcard_value(value, contact->related);
        } else if (strcmp(property, "EXPERTISE") == 0) {
            unescape_vcard_value(value, contact->expertise);
        } else if (strcmp(property, "HOBBY") == 0) {
            unescape_vcard_value(value, contact->hobby);
        } else if (strcmp(property, "INTEREST") == 0) {
            unescape_vcard_value(value, contact->interest);
        }
    }
    
    // Set creation time if not set
    if (strlen(contact->created) == 0) {
        strcpy(contact->created, contact->rev);
    }
}

void save_contacts(ContactList* list) {
    FILE* file = fopen("contacts.vcf", "w");
    if (!file) return;
    
    for (int i = 0; i < list->count; i++) {
        write_vcard_contact(file, &list->contacts[i]);
    }
    
    fclose(file);
}

void load_contacts(ContactList* list) {
    FILE* file = fopen("contacts.vcf", "r");
    if (!file) return;
    
    char line[256];
    list->count = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "BEGIN:VCARD", 11) == 0) {
            if (list->count < MAX_CONTACTS) {
                read_vcard_contact(file, &list->contacts[list->count]);
                list->count++;
            }
        }
    }
    
    fclose(file);
}

// Contact management functions
void sort_contacts(ContactList* list) {
    // Simple bubble sort by full name
    for (int i = 0; i < list->count - 1; i++) {
        for (int j = 0; j < list->count - i - 1; j++) {
            if (_stricmp(list->contacts[j].fn, list->contacts[j + 1].fn) > 0) {
                Contact temp = list->contacts[j];
                list->contacts[j] = list->contacts[j + 1];
                list->contacts[j + 1] = temp;
            }
        }
    }
}

void search_contacts(ContactList* list, const char* search_term) {
    ContactList* temp_list = (ContactList*)malloc(sizeof(ContactList));
    if (temp_list == NULL) {
        // Not enough memory, cannot perform search.
        return;
    }
    memset(temp_list, 0, sizeof(ContactList));
    
    for (int i = 0; i < list->count; i++) {
        Contact* c = &list->contacts[i];
        
        // Search in multiple fields (case-insensitive)
        if (stristr(c->fn, search_term) ||
            stristr(c->nickname, search_term) ||
            stristr(c->org, search_term) ||
            stristr(c->note, search_term) ||
            stristr(c->categories, search_term)) {
            
            temp_list->contacts[temp_list->count++] = *c;
            continue;
        }
        
        // Search in emails
        for (int j = 0; j < c->email_count; j++) {
            if (stristr(c->emails[j].value, search_term)) {
                temp_list->contacts[temp_list->count++] = *c;
                break;
            }
        }
    }
    
    // Copy the results back to the original list structure
    memcpy(list->contacts, temp_list->contacts, temp_list->count * sizeof(Contact));
    list->count = temp_list->count;
    list->selected = 0;
    list->scroll_offset = 0;

    free(temp_list);
}

// Input handling
void edit_field(char* field, int max_length) {
    char temp[MAX_LONG_FIELD_LEN];
    strcpy(temp, field);
    
    // Show cursor
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    
    int pos = strlen(temp);
    int key;
    
    while (1) {
        // Redraw field
        printf("\r%-*s\r%s", max_length + 10, "", temp);
        
        key = _getch();
        
        if (key == 13) { // Enter
            strcpy(field, temp);
            break;
        } else if (key == 27) { // ESC
            break;
        } else if (key == 8) { // Backspace
            if (pos > 0) {
                temp[--pos] = '\0';
            }
        } else if (key >= 32 && key < 127 && pos < max_length - 1) {
            temp[pos++] = key;
            temp[pos] = '\0';
        }
    }
    
    // Hide cursor
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

int handle_list_input(ContactList* list) {
    int key = _getch();
    int list_height = WINDOW_HEIGHT - 6;
    
    if (key == 27) { // ESC
        return 1; // Exit program
    } else if (key == 224) { // Arrow keys
        key = _getch();
        switch (key) {
            case 72: // Up
                if (list->selected > 0) {
                    list->selected--;
                    if (list->selected < list->scroll_offset) {
                        list->scroll_offset = list->selected;
                    }
                }
                break;
            case 80: // Down
                if (list->selected < list->count - 1) {
                    list->selected++;
                    if (list->selected >= list->scroll_offset + list_height) {
                        list->scroll_offset = list->selected - list_height + 1;
                    }
                }
                break;
            case 73: // Page Up
                list->selected -= list_height;
                if (list->selected < 0) list->selected = 0;
                list->scroll_offset = list->selected;
                break;
            case 81: // Page Down
                list->selected += list_height;
                if (list->selected >= list->count) list->selected = list->count - 1;
                if (list->selected >= list->scroll_offset + list_height) {
                    list->scroll_offset = list->selected - list_height + 1;
                }
                break;
        }
    } else if (key == 13) { // Enter - View details
        if (list->count > 0) {
            list->current_contact = &list->contacts[list->selected];
            list->mode = VIEW_DETAIL;
        }
    } else if (key == 'n' || key == 'N') { // New contact
        list->mode = VIEW_NEW;
        memset(&list->edit_buffer, 0, sizeof(Contact));
        strcpy(list->edit_buffer.kind, "individual");
        list->edit_field = 0;
    } else if (key == 's' || key == 'S') { // Search
        list->mode = VIEW_SEARCH;
    } else if (key == 'r' || key == 'R') { // Refresh/reload
        load_contacts(list);
        sort_contacts(list);
    }
    
    return 0;
}

int handle_detail_input(ContactList* list) {
    int key = _getch();
    
    if (key == 27) { // ESC - Back to list
        list->mode = VIEW_LIST;
    } else if (key == 'e' || key == 'E') { // Edit
        list->mode = VIEW_EDIT;
        list->edit_buffer = *list->current_contact;
        list->edit_field = 0;
    } else if (key == 'd' || key == 'D') { // Delete
        // Find and remove the contact
        for (int i = 0; i < list->count; i++) {
            if (&list->contacts[i] == list->current_contact) {
                // Shift remaining contacts
                for (int j = i; j < list->count - 1; j++) {
                    list->contacts[j] = list->contacts[j + 1];
                }
                list->count--;
                if (list->selected >= list->count && list->selected > 0) {
                    list->selected--;
                }
                save_contacts(list);
                list->mode = VIEW_LIST;
                break;
            }
        }
    }
    
    return 0;
}

int handle_edit_input(ContactList* list) {
    int key = _getch();
    int total_fields = 40; // Adjust based on actual field count
    
    if (key == 27) { // ESC - Cancel
        list->mode = (list->current_contact != NULL) ? VIEW_DETAIL : VIEW_LIST;
    } else if (key == 224) { // Arrow keys
        key = _getch();
        switch (key) {
            case 72: // Up
                if (list->edit_field > 0) list->edit_field--;
                break;
            case 80: // Down
                if (list->edit_field < total_fields - 1) list->edit_field++;
                break;
        }
    } else if (key == 13) { // Enter - Edit current field
        Contact* c = &list->edit_buffer;
        
        // Map field index to actual field
        switch (list->edit_field) {
            case 0: edit_field(c->fn, 50); break;
            case 1: edit_field(c->prefix, 20); break;
            case 2: edit_field(c->given, 30); break;
            case 3: edit_field(c->family, 30); break;
            case 4: edit_field(c->nickname, 30); break;
            // Email fields (5-9)
            case 5: case 6: case 7: case 8: case 9: {
                int email_idx = list->edit_field - 5;
                if (email_idx >= c->email_count && email_idx < MAX_EMAILS) {
                    c->email_count = email_idx + 1;
                    strcpy(c->emails[email_idx].type, "WORK");
                }
                edit_field(c->emails[email_idx].value, 40);
                break;
            }
            // Phone fields (10-14)
            case 10: case 11: case 12: case 13: case 14: {
                int phone_idx = list->edit_field - 10;
                if (phone_idx >= c->phone_count && phone_idx < MAX_PHONES) {
                    c->phone_count = phone_idx + 1;
                    strcpy(c->phones[phone_idx].type, "WORK");
                }
                edit_field(c->phones[phone_idx].value, 30);
                break;
            }
            case 15: edit_field(c->org, 50); break;
            case 16: edit_field(c->title, 40); break;
            case 17: edit_field(c->role, 40); break;
            case 18: edit_field(c->bday, 20); break;
            case 19: edit_field(c->anniversary, 20); break;
            case 20: edit_field(c->lang, 20); break;
            case 21: edit_field(c->tz, 30); break;
            // Address fields (22-26)
            case 22: edit_field(c->addresses[0].street, 40); break;
            case 23: edit_field(c->addresses[0].locality, 30); break;
            case 24: edit_field(c->addresses[0].region, 20); break;
            case 25: edit_field(c->addresses[0].postalcode, 15); break;
            case 26: edit_field(c->addresses[0].country, 30); break;
            case 27: edit_field(c->url, 50); break;
            case 28: edit_field(c->linkedin, 40); break;
            case 29: edit_field(c->twitter, 30); break;
            case 30: edit_field(c->impp, 40); break;
            case 31: edit_field(c->birthplace, 40); break;
            case 32: edit_field(c->birthplace_geo, 30); break;
            case 33: edit_field(c->deathdate, 20); break;
            case 34: edit_field(c->deathplace, 40); break;
            case 35: edit_field(c->deathplace_geo, 30); break;
            case 36: edit_field(c->related, 40); break;
            case 37: edit_field(c->expertise, 40); break;
            case 38: edit_field(c->hobby, 40); break;
            case 39: edit_field(c->interest, 40); break;
            case 40: edit_field(c->categories, 40); break;
            case 41: edit_field(c->note, 50); break;
        }
    } else if (key == 's' || key == 'S') { // Save
        update_revision(&list->edit_buffer);
        
        if (list->mode == VIEW_NEW) {
            // Add new contact
            if (list->count < MAX_CONTACTS) {
                list->contacts[list->count++] = list->edit_buffer;
                sort_contacts(list);
                // Find the newly added contact
                for (int i = 0; i < list->count; i++) {
                    if (strcmp(list->contacts[i].uid, list->edit_buffer.uid) == 0) {
                        list->selected = i;
                        break;
                    }
                }
            }
        } else {
            // Update existing contact
            *list->current_contact = list->edit_buffer;
            sort_contacts(list);
        }
        
        save_contacts(list);
        list->mode = VIEW_LIST;
    }
    
    return 0;
}

int main() {
    // Store original console settings
    OriginalConsoleSettings originalSettings;
    store_original_settings(&originalSettings);
    
    // Set console to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitle("VCard Contact Manager");
    
    // Setup console size
    setup_console_size();
    
    // Hide cursor
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    
    // Initialize contact list on the heap
    ContactList* list = (ContactList*)malloc(sizeof(ContactList));
    if (list == NULL) {
        perror("Failed to allocate memory for contact list");
        return 1;
    }
    memset(list, 0, sizeof(ContactList));
    list->mode = VIEW_LIST;
    
    // Load contacts
    load_contacts(list);
    sort_contacts(list);
    
    ViewMode previous_mode = -1;

    // Main loop
    while (1) {
        if (list->mode != previous_mode) {
            clear_screen();
            previous_mode = list->mode;
        }

        switch (list->mode) {
            case VIEW_LIST:
                draw_title_bar("VCard Contact Manager - Contact List");
                draw_borders();
                draw_contact_list(list);
                draw_status_bar("↑/↓ Navigate | Enter: View | N: New | S: Search | R: Refresh | ESC: Exit");
                if (handle_list_input(list)) goto exit;
                break;
                
            case VIEW_DETAIL:
                draw_contact_detail(list->current_contact);
                handle_detail_input(list);
                break;
                
            case VIEW_EDIT:
            case VIEW_NEW:
                draw_edit_form(&list->edit_buffer, list->edit_field);
                handle_edit_input(list);
                break;
                
            case VIEW_SEARCH:
                // Simple search implementation
                draw_title_bar("Search Contacts");
                draw_borders();
                gotoxy(5, 5);
                set_color(FOREGROUND_WHITE);
                printf("Enter search term: ");
                
                cursorInfo.bVisible = TRUE;
                SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
                
                char search_term[MAX_FIELD_LEN] = "";
                edit_field(search_term, 50);
                
                cursorInfo.bVisible = FALSE;
                SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
                
                if (strlen(search_term) > 0) {
                    ContactList* original_list = (ContactList*)malloc(sizeof(ContactList));
                    if (original_list == NULL) {
                        // Not enough memory to perform search, just return to list
                        list->mode = VIEW_LIST;
                        break;
                    }
                    memcpy(original_list, list, sizeof(ContactList));

                    search_contacts(list, search_term);
                    list->mode = VIEW_LIST;
                    
                    // Wait for any key to return to full list
                    clear_screen();
                    draw_title_bar("Search Results");
                    draw_borders();
                    draw_contact_list(list);
                    draw_status_bar("Press any key to return to full list");
                    _getch();
                    
                    memcpy(list, original_list, sizeof(ContactList));
                    free(original_list);
                }
                list->mode = VIEW_LIST;
                break;
        }
    }
    
exit:
    // Save before exit
    save_contacts(list);
    
    // Free the contact list
    free(list);
    
    // Restore original console settings
    restore_original_settings(&originalSettings);
    
    return 0;
}