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
#include "usbmonitor.h"

#define USAGE "./usbmonitor [-h host] [-p port]"
#define TIME_DELTA 5
#define ROTATION_DELTA 20

static time_t w_start;
static FILE *file;
static Hashtable* macaddrs;
static char filename[30];
static int must_exit = 0;

static void *handler(void *args) {
    //the place that will listen on usb plugged in or removed!
	
	/*char event[SOCK_RECV_BUF_LEN], resp[100];
	char stsmsg[RTAB_MSG_MAX_LENGTH];
	RpcConnection sender;
	unsigned len, rlen;
	unsigned long i;
	Rtab *results;
	UserEventResults *ue;

	while ((len = rpc_query(rps, &sender, event, SOCK_RECV_BUF_LEN)) > 0) {
		sprintf(resp, "OK");
		rlen = strlen(resp) +1;
		rpc_response(rps, sender, resp, rlen);
		event[len] = '\0';
		results = rtab_unpack(event, len);
		if (results && ! rtab_status(event, stsmsg)) {
			ue = mon_convert(results);
			for (i = 0; i < ue->nevents; i++) {
				UserEvent *f = ue->data[i];
				char *s = timestamp_to_string(f->tstamp);
				printf("%s %s %s %s\n", s, f->application, f->logtype, f->logdata);
				if (strcmp(f->logtype, "NOX_PLUGGED") == 0)
					handle_usb_plugged(f->logtype, f->logdata);
				else if (strcmp(f->logtype, "NOX_UNPLUGGED") == 0){
					handle_usb_unplugged(f->logtype, f->logdata);
				}
				free(s);
			}
			mon_free(ue);
		}
		rtab_free(results);
	}
	return (args) ? NULL : args;	 unused warning subterfuge */
}

void handle_usb_unplugged(char* type, char* data){
	must_exit = 1;
	pid_t pid = fork();
	char device[128];
	sprintf(device, "/dev/%s", data);

	if (pid ==0){ /*child process*/	
		
		execl("/bin/umount", "umount", "-l", device, (char *)0);
	}else{
		waitpid(pid,0,0);
		
		MacRec *head = (MacRec *) ht_lookup(macaddrs, device);
		if (head != NULL){
			char noxurl[60];
			strcpy(noxurl, "http://localhost/ws.v1/homework/deny/");
			//noxpost(noxurl, head->addr);
			MacRec *current = head;
			while ( (current = current->next) != NULL){
				char noxurl[60];
				strcpy(noxurl, "http://localhost/ws.v1/homework/deny/");
				//noxpost(noxurl, current->addr);
			}
			ht_remove(macaddrs, device);
			free_macrec(head);
		}
	}
}


void handle_usb_plugged(char* type, char* data){
	pid_t pid = fork();
	pthread_t thr;
	char device[128];
	char mountpoint[128];
	must_exit = 0;
	
	sprintf(device, "/dev/%s", data);
	sprintf(mountpoint, "/mnt");

	if (pid ==0){ /*child process*/
		printf("mounting...%s\n", device);
		mkdir(mountpoint, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		execl("/bin/mount", "mount", "-o", "sync", device, mountpoint, (char *)0);
	}else{ /*pid !=0 parent process*/
		waitpid(pid,0,0);
		DIR *dp;
		struct dirent *ep;
		printf("mounted..\n");
		dp = opendir(mountpoint);
		if (dp != NULL){
			MacRec* previous = NULL;
			MacRec* head = NULL;
			
			
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
					char noxurl[60];
					strcpy(noxurl, "http://localhost/ws.v1/homework/permit/");
					//noxpost(noxurl , ep->d_name);
				} 

			}
			ht_insert(macaddrs, device, head);
			(void) closedir(dp);
		}else{
			perror("couldn't open the directory!");
		}	   
	}
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
	
	while(true){
	    
	    dbus_connection_read_write(connection,-1); //block, waiting on message
	    
	    while( (msg = dbus_connection_pop_message(connection)) != NULL){
	    //if (NULL == msg){
	     //   sleep(1);
	     //   continue;
	   // }
	  
            if (dbus_message_is_signal(msg, "test.signal.Type", "Test")){
                printf("seen a signal!!\n");
                if (!dbus_message_iter_init(msg, &args))
                    fprintf(stderr, "message has no parameters!\n");
                else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
                     fprintf(stderr, "arg is not a string!\n");
                else
                    dbus_message_iter_get_basic(&args, &sigvalue);
                    printf("got signal with value %s\n", sigvalue);
            }
            dbus_message_unref(msg);
	    }
	}
    

	macaddrs = ht_create(100);
    /*pthread_t thr;
	/* start handler thread 
	if (pthread_create(&thr, NULL, handler, NULL)) {
		fprintf(stderr, "Failure to start handler thread\n");
		exit(-1);
	}*/
}

