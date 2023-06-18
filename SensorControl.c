#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "SensorControl.h"



static unsigned long Status_Register; 
static unsigned long Config_Register;

static struct control_CB    BT_control_cb;

static int val_1 = 0; 
static int val_2 = 0;  




/*
CALLBACK READ FUNCTIONS

Callback Functions below follow the bt_gatt_attr_read_func_t and write function signature
Which is that the function returns a ssize_T and takes the connection, attribute
buffer, length and offset arguments

First the 3 Read functions are defined, and last the write function.
*/




static ssize_t Give_Sensor_Status  ( struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             void *buf,
                             uint16_t len,
                             uint16_t offset)
{
    
    const char *value = attr->user_data;
    if (BT_control_cb.Status) 
    {
        Status_Register = BT_control_cb.Status(); //This .status function pointer will be assigned 
                                                  //a function in main, and from there actually give this
                                                 //variable a value which will then be collected by 
                                                 // the read below vvv  
        return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                     sizeof(*value));
    }
    return 0;
}



static ssize_t read_val_1  ( struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             void *buf,
                             uint16_t len,
                             uint16_t offset)
{
    //get a pointer to button_state which is passed in the BT_GATT_CHARACTERISTIC() and stored in attr->user_data
    const int *value = attr->user_data; 
    printk("Now you are in the temp reader function\n");
    if (BT_control_cb.Value_1) {
        printk("You even made it to the function call\n");
        // Call the application callback function to update the get the current value of the button
        val_1 = BT_control_cb.Value_1(); //This .status function pointer will be assigned 
                                        //a function in main, and from there actually give this
                                        //variable a value which will then be collected by 
                                        //    the read below vvv


        printk("If you made it here the number should have updated.\n");
        printk("The value of value_1 is %d\n", val_1); 
        printk("The value of the attribute value, or at least the value at the pointer is: %d", *value);
        printk("The value in the attribute itself is %d", attr->user_data);
        /*
        Above prints are good for understanding how the code around attributes works but can be commented out
        or removed if that is not needed. 
        */

        return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                     sizeof(*value));
    }
    return 0;
}

static ssize_t read_val_2  ( struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             void *buf,
                             uint16_t len,
                             uint16_t offset)
{
    const char *value = attr->user_data;
    if (BT_control_cb.Value_2) 
    {
        val_2 = BT_control_cb.Value_2(); 
        return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                     sizeof(*value));
    }
    return 0;
}





/*
WRITE CALLBACK FUNCTIONS

Below are the write functions. Because they simply call a function from main with the value
provided by the user, the characteristics they belong to do not have attribute values they display.
*/

static ssize_t Set_Register (struct bt_conn *conn,
                    const struct bt_gatt_attr *attr,
                    const void *buf,
                    uint16_t len, uint16_t offset, uint8_t flags)
{
    if ( len > 1) { //testing 
        printk("Write Control: Incorrect data length");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN); 
    }      //vvv if you are checking for anything else here you are probably making it too complicated
    if (offset != 0 ) {
        printk("Write Control: Incorrect data offset");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    if (BT_control_cb.ControlInput) {
        //Below it reads the received value if it passes the checks 
        uint8_t val = *((uint8_t *)buf); //The value that is interesting to read is in this buffer
        
        if (((val == 1) || (val == 2)) || (val == 4)) 
        {  //The Write function's accepted span
           //Call the application callback function to update the LED state
            (BT_control_cb.ControlInput)(val); 
        } else {                                 
            printk("Write: Incorrect value");
            return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
        }
    }
    return len;
}

static ssize_t Configure_Register (struct bt_conn *conn,
                    const struct bt_gatt_attr *attr,
                    const void *buf,
                    uint16_t len, uint16_t offset, uint8_t flags)
{



    if ( len > 1) 
    { 
        printk("Write Config: Incorrect data length");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN); 
    }      // if you are checking for anything else for offset you are probably making it too complicated
    if (offset != 0 ) 
    {
        printk("Write Config: Incorrect data offset");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    if (BT_control_cb.Config) 
    {
        //Below it reads the received value if it passes the checks 
        uint8_t val = *((uint8_t *)buf); //The value that is interesting to read is in this buffer
        
        if (!(val == 0) && !(val > 2))  
        {  //The Write function's accepted span
           //Call the application callback function to update the state
            (BT_control_cb.Config)(val); //The actual app function in main does the actual writing
        }
        else 
        {                                 
            printk("Write: Incorrect value");
            return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
        }
    }
    return len;
}


BT_GATT_SERVICE_DEFINE(SensorControlService,BT_GATT_PRIMARY_SERVICE(BT_UUID_SENSORCONTROL_SERVICE),BT_GATT_CHARACTERISTIC(BT_UUID_CONTROL, BT_GATT_CHRC_WRITE,BT_GATT_PERM_WRITE, NULL,Set_Register, NULL),BT_GATT_CHARACTERISTIC(BT_UUID_STATUS, BT_GATT_CHRC_READ,BT_GATT_PERM_READ, Give_Sensor_Status,NULL, &Status_Register),BT_GATT_CHARACTERISTIC(BT_UUID_TEMP_VALUE, BT_GATT_CHRC_READ,BT_GATT_PERM_READ, read_val_1,NULL, &val_1),BT_GATT_CHARACTERISTIC(BT_UUID_MOIST_VALUE, BT_GATT_CHRC_READ,BT_GATT_PERM_READ, read_val_2,NULL, &val_2),BT_GATT_CHARACTERISTIC(BT_UUID_CONFIG, BT_GATT_CHRC_WRITE,BT_GATT_PERM_WRITE, NULL,Configure_Register, NULL));



int SensorControlInit (struct control_CB *callbacks)
{
    if (callbacks)
    {
        BT_control_cb.Status        = callbacks->Status;
        BT_control_cb.ControlInput  = callbacks->ControlInput;
        BT_control_cb.Value_1       = callbacks->Value_1;
        BT_control_cb.Value_2       = callbacks->Value_2;
        BT_control_cb.Config        = callbacks->Config;
    }

    return 0;

}
