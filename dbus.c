#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

int main(int argc, char** argv)
{
	DBusConnection *connection;
	DBusMessage *msg;
	DBusMessage *reply;
	DBusMessageIter args;
	//DBusGProxy *proxy;
	DBusError err;
	char* sigvalue;
	//char **name_list;
 	//char **name_list_ptr;
    int ret;
    
	//g_type_init();
	dbus_error_init(&err);
	
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (connection == NULL){
		printf("Failed to open connection to bus: %s\n", err.message);
		dbus_error_free(&err);
		exit(1);
	}
	
	ret = dbus_bus_request_name(connection, "test.signal.sink", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
	
	if (dbus_error_is_set(&err)){
	    fprintf(stderr, "Name error (%s)\n", err.message); 
	    dbus_error_free(&err);
	}
	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret){
	    printf("not primary owner!!\n");
	    exit(1);
	}
	
	printf("lovely job - got here!!\n ");
	dbus_bus_add_match(connection, "type='signal',interface='test.signal.Type'", &err);
	dbus_connection_flush(connection);

	if (dbus_error_is_set(&err)){
	    fprintf(stderr, "error (%s)\n", err.message); 
	    exit(1);
	}
	
	printf("Match rule sent!!\n");
	
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
}
