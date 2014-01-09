typedef struct macrec{
	struct macrec *next;
	char addr[18];
} MacRec;

static void rotatefile();
void replace_char(char *str, char from, char to);
void handle_usb_plugged(char* type, char* data);
void handle_usb_unplugged(char* type, char* data);
int match(const char *string, char *pattern);
void free_macrec(MacRec *head);
