/*
* USB callback client for the Homework Database.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include <signal.h>

#include <pthread.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>
#include <regex.h>
#include "hashtable.h"
#include "usbsignaller.h"

#define USAGE "./usbsignaller [-h host] [-p port]"
#define TIME_DELTA 5
#define ROTATION_DELTA 20

static time_t w_start;
static FILE *file;
static Hashtable* macaddrs;
static char filename[30];
static int must_exit = 0;

void handle_usb_unplugged(char* device){
	must_exit = 1;
	pid_t pid = fork();

	if (pid ==0){ /*child process*/			
		execl("/bin/umount", "umount", "-l", device, (char *)0);
	}else{
		waitpid(pid,0,0);
		
		MacRec *head = (MacRec *) ht_lookup(macaddrs, device);
		char* signal =  (char*) malloc(32);
		
		if (head != NULL){
			
			sprintf(signal, head->addr, 0, 17);
			strcat(signal, " deny");
			sendsignal(signal);
			MacRec *current = head;
			
			while ( (current = current->next) != NULL){
			    printf("deny --> %s\n", current->addr);
				sprintf(signal, current->addr, 0, 17);
				strcat(signal, " deny");
				sendsignal(signal);
			}
			ht_remove(macaddrs, device);
			free_macrec(head);
			free(signal);
		}
	}
}

void handle_usb_plugged(char* device){
	pid_t pid = fork();
	pthread_t thr;
	//char device[128];
	char mountpoint[128];
	must_exit = 0;
	
	//sprintf(device, "/dev/%s", data);
	sprintf(mountpoint, "/mnt");

	if (pid ==0){ /*child process*/
		printf("mounting...%s\n", device);
		mkdir(mountpoint, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		execl("/bin/mount", "mount", "-o", "sync", device, mountpoint, (char *)0);
		
	}else{ /*pid !=0 parent process*/
		waitpid(pid,0,0);
		DIR *dp;
		struct dirent *ep;
		printf("mounted!!, opening %s..\n", mountpoint);
		dp = opendir(mountpoint);
		printf("opened dir\n");
		if (dp != NULL){
			MacRec* previous = NULL;
			MacRec* head = NULL;
			char* signal =  (char*) malloc(32);
			
			while (ep = readdir(dp)){
				if (match(ep->d_name, "^([[:xdigit:]]{1,2}\\-){5}[[:xdigit:]]{1,2}")){
					replace_char(ep->d_name, '-', ':');
					MacRec* m = (MacRec *)malloc(sizeof(MacRec));
					m->next = NULL;
					sprintf(m->addr, "%s", ep->d_name);
					if (previous != NULL){
						previous->next = m;
					}else{
						head = m;
					}
					previous = m;
					
					//signal = "";
					sprintf(signal, ep->d_name, 0, 17);
					strcat(signal, " permit");
					printf("signal is %s\n", signal);
					sendsignal(signal);
					printf("would permit %s\n", ep->d_name);
				}
			}
			if (head){ //if any matches found
                printf("adding %s to macrec!\n", head->addr);
                ht_insert(macaddrs, device, head);
            }
			
			free(signal);
			(void) closedir(dp);
		}else{
			perror("couldn't open the directory!");
		}	   
	}
}

void sendsignal(char* sigvalue){
   
    DBusMessage *msg;
    DBusMessageIter args;
    DBusConnection* conn;
    DBusError err;
    int ret;
    dbus_uint32_t serial = 0;
    
    printf("sending signal with value [%s]\n", sigvalue);
    dbus_error_init(&err);
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)){
        fprintf(stderr, "Connection error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (NULL == conn){
        exit(1);
    }
    
    ret = dbus_bus_request_name(conn, "test.signal.usb", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err)){
        fprintf(stderr, "Name error (%s)\n", err.message);
        dbus_error_free(&err);
    }
  
    msg = dbus_message_new_signal("/test/signal/Object", "test.signal.Type", "udev");
    
    if (NULL == msg){
        fprintf(stderr, "message null\n");
        exit(1);
    }
    
    dbus_message_iter_init_append(msg, &args);
    
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &sigvalue)){
        printf("out of memory!\n");
        exit(1);
    }
    
    if (!dbus_connection_send(conn,msg, &serial)){
        printf("out of memory!\n");
        exit(1);
    }
    dbus_connection_flush(conn);
    printf("signal sent!!");
}

static void rotatefile(){
	printf("closing %s\n", filename);
	fclose(file);
	sprintf(filename, "%s%u%s","/mnt/data",time(NULL), ".txt");	
	file = fopen(filename,"a+");
}


void free_macrec(MacRec *head){
	MacRec *current = head;

	while (current != NULL){
		MacRec *next = current->next;
		free(current);
		current = next;
	}
}


void replace_char(char *str, char from, char to){
	for (; *str; ++ str){
		if (*str == from)
			*str = to;
	}
}

int match(const char *string, char *pattern){
	int status;
	regex_t re;
	if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0){
		return 0;
	}
	status = regexec(&re, string, (size_t)0, NULL, 0);
	regfree(&re);
	if (status!=0){
		return 0;
	}
	return 1;
}

int main(int argc, char *argv[]) {
  
    DBusConnection *connection;
	DBusMessage *msg;
	DBusMessage *reply;
	DBusMessageIter args;
	DBusError err;
	char* sigvalue;
    int ret;

	dbus_error_init(&err);
	
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (connection == NULL){
		printf("Failed to open connection to bus: %s\n", err.message);
		dbus_error_free(&err);
		exit(1);
	}
	
	ret = dbus_bus_request_name(connection, "test.signal.usb", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
	
	if (dbus_error_is_set(&err)){
	    fprintf(stderr, "Name error (%s)\n", err.message); 
	    dbus_error_free(&err);
	}
	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret){
	    printf("not primary owner!!\n");
	    exit(1);
	}
	
	printf("listening for usb additions\n ");
	//dbus_bus_add_match(connection, "type='signal',interface='test.signal.Type'", &err);
	dbus_bus_add_match(connection, "type='signal',interface='test.signal.Type'", &err);
	dbus_connection_flush(connection);

	if (dbus_error_is_set(&err)){
	    fprintf(stderr, "error (%s)\n", err.message); 
	    exit(1);
	}

	macaddrs = ht_create(100);
	
	while(true){
	    printf("creating dbus connection\n");
	    dbus_connection_read_write(connection,-1); //block, waiting on message
	    printf("seen a message!!\n");
	    while( (msg = dbus_connection_pop_message(connection)) != NULL){
            if (dbus_message_is_signal(msg, "test.signal.Type", "device")){
                printf("seen a signal!!\n");
                if (!dbus_message_iter_init(msg, &args)){
                    fprintf(stderr, "message has no parameters!\n");
                }
                else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)){
                     fprintf(stderr, "arg is not a string!\n");
                }
                else{
                    dbus_message_iter_get_basic(&args, &sigvalue);
                    printf("got signal %s\n", sigvalue);
                    int argc = 0;
                    char *token;
                    char command[7];
                    char device[18];
                    
                    if (sigvalue != NULL){
                        while ((token = strsep(&sigvalue, " ")) != NULL){
                            if (argc == 0){
                                sprintf(command, token, 0, 7);
                            }
                            if (argc == 1){
                                sprintf(device, token, 0, 15);
                            }
                            argc++;
                        }
                    }
                    if (argc == 3){
                        printf("read in %s %s args\n", command, device);
                        if (strcmp(command, "add") == 0){
                            handle_usb_plugged(device);
                        }else if (strcmp(command, "remove") == 0){
                            handle_usb_unplugged(device);
                        }
                    }
                }
                    //split up the parameter and call appropriate method (should this not be done with dbus method??)
            }
            dbus_message_unref(msg);
	    }
	}
}