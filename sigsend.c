#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>


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
    //if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret){
     //   printf("not primary owner!!\n");
      //  exit(1);
        
    //}
    
    msg = dbus_message_new_signal("/test/signal/Object", "test.signal.Type", "device");
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

int main(int argc, char** argv)
{
    int msglen = 0;
    
    if (argc > 1){
        for (int i = 1; i < argc; i++){
            msglen += 1 + strlen(argv[i]);    
        }
    
        char* message = (char*) malloc(msglen + 1);
       
        for (int i =1; i < argc; i++){
            strcat(strcat(message, argv[i]), " ");
        }
    
        printf("%s\n", message);
        sendsignal(message);
        free(message);
    }
}
