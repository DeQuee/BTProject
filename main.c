//Detta är framtida main applikation för bluetooth Control och Status-lager. 

#include "SensorControl.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

#define RUN_LED_BLINK_INTERVAL  5000

#define THREAD_STACK_SIZE       5000   





/*
Below are the variables which are used in the callback functions.
*/

static int startSens = 0;
static int stopSens = 0;
static int sensorStatus = 0;
static bool measureVal1 = 0;
static bool measureVal2 = 0;
static unsigned long user_control_register;
static unsigned long user_config_register;
static int dummy_Val_1 = 0;
static int dummy_Val_2 = 0;
static struct sensor_value Val_1; //The volatile sensor values are currently saved as
static struct sensor_value Val_2; //a struct, from an earlier version. which had an integer and fractional value. But it can be anything
static int safeVal_1;             //that works with the function. For now we use the dummy values instead.
static int safeVal_2;
static bool app_button_state;
//const struct device *const dht11 = DEVICE_DT_GET_ONE(aosong_dht);
    //this registers the dht sensor, this also requires some extra text
    //found in the overlay. 

//standard advertisement setup
static const struct bt_data ad[] = 
{
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

//standard scan setup
static const struct bt_data sd[] = 
{
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_SENSORCONTROL_SERVICE_val),
};




//A sensorfunction that creates a 16 char buffer with current time and prints.
//Not currently used but useful regardless if one wants values with timestamps outside
//of logging.   
static const char *now_str(void)
{
    static char buf[16]; /* ...HH:MM:SS.MMM */
    uint32_t now = k_uptime_get_32();
    unsigned int ms = now % MSEC_PER_SEC;
    unsigned int s;
    unsigned int min;
    unsigned int h;

    now /= MSEC_PER_SEC;
    s = now % 60U;
    now /= 60U;
    min = now % 60U;
    now /= 60U;
    h = now;

    snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u",
         h, min, s, ms);
    return buf;
}

//advertising parameters are set through a simple macro. 
static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM((BT_LE_ADV_OPT_CONNECTABLE|BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
                800, /*Min Advertising Interval 500ms (800*0.625ms) */
                801, /*Max Advertising Interval 500.625ms (801*0.625ms)*/
                NULL); /* Set to NULL for undirected advertising*/

/*
CONNECTION CALLBACK FUNCTIONS
-----------------------------------------------------------------------------
*/


static void on_connected(struct bt_conn *conn, uint8_t err)
{


    if (err) 
    {
        printk("Connection failed (err %u)\n", err);
        return;
    }

    
    printk("Connected\n");
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{


    printk("Disconnected (reason %u)\n", reason);
}
/*
END OF CONNECTION CALLBACKS----------------------------------------------------------------------------------
*/



/*
WORK ITEMS SUBMITTED BY THE CONTROL REGISTER ||||| ---------------------------------------------------------------
                                             vvvvv
*/

static void app_Value_1_Write_impl(struct k_work work) 
{


    printk("The Value_1 work item has been started\n");

    startSens = 1; //Currently symbolic, the idea is that it means the sensor is busy insofar as other callbacks are concerned.  

    if (!measureVal1)
    {
        printk("The Value_1 measurement is not enabled");
        startSens = 0; //important that we mark that the sensor is no longer busy. 
        return 0;

    }


    printk("The sensor is ready, starting Value_1 measurement\n");


    dummy_Val_1++;
    safeVal_1 = dummy_Val_1;

    return;

}

static void app_Value_2_Write_impl(struct k_work work)  
{                                                       



   printk("The Value_2 work item has been reached\n");


    startSens = 1;
    if (!measureVal2)   //This tests for if the user has toggled this type of measurement. 
    {                   //Largely symbolic of desire for user configuration and maybe not necessary in this form 
                        //depending on final product. 
                        //But some configuration from the user end will no doubt be part of it. 

        printk("The Value_2 measurement is not enabled\n");
        startSens = 0;
        return;
    
    }    


    printk("The sensor is ready, starting Value_2 measurement\n");


    dummy_Val_2++; //dummy measurement
    safeVal_2 = dummy_Val_2;    
    startSens = 0;


    return;  

}



/*
END OF WORK ITEMS -----------------------------------------------------------------
*/

//K_WORK_DEFINES OF WORK ITEMS.
static K_WORK_DEFINE(Val_1_update, app_Value_1_Write_impl);
static K_WORK_DEFINE(Val_2_update, app_Value_2_Write_impl);

/*
APPLICATION CALLBACK FUNCTIONS
-------------------------------------------------------------------------------------------------------------
*/

static void app_control(uint8_t val)
{


    printk("You have now set the value of the control register to %lu\n", user_control_register);
    

    user_control_register = (user_control_register | val);
    
    switch (user_control_register)
    {
        case MeasureTemperature:
            printk("Start measuring Value_1\n"); 
            k_work_submit(&Val_1_update);  
            break;         
        case MeasureMoisture:                        
            printk("Start measuring Value_2\n");
            k_work_submit(&Val_2_update);
            break;
        default:
            printk("Incorrect sensor input\n");
    }
    user_control_register = 0;

}

static void app_configure(uint8_t val)
{


    user_config_register = val; //adds the value to the register, bitwise OR since we want to keep the config
    
    switch (user_config_register)
    {
        case ToggleTemperature:
            measureVal1 = !measureVal1;
            printk("MeasureVal1 is = %d", measureVal1);
            break;
        case ToggleMoisture:
            measureVal2 = !measureVal2;
            printk("MeasureVal2 is = %d", measureVal2);
            break;
        default:
            printk("Incorrect config input, user_config = %lu\n", user_config_register);
    
    } 
}

static uint8_t app_status()
{


    return (uint8_t)startSens; //should be simple enough for now

}


static int app_Read_Function_1()
{
    if (startSens == 0)
    {


        return safeVal_1;
    
    }
    else
    {


        printk("The sensor is busy currently\n");
        return 0;

    }
}



static int app_Read_Function_2()
{
    if (startSens == 0)
    {


        return safeVal_2;
    
    }
    else
    {


        printk("The sensor is busy currently\n");
        return 0;
        
    }
}


/*
END OF APPLICATION CALLBACK FUNCTIONS-----------------------------------------------------------------------------
*/



/*
Below, a callback struct is created in the application. The functions listed in
it are connected to the struct in the SensorControl.c file through
the init function in the main area which uses function pointers to do so.
*/


struct bt_conn_cb connection_callbacks = 
{
    .connected              = on_connected,
    .disconnected           = on_disconnected,  
};

struct control_CB control_callbacks = 
{ 
        .Status = app_status,
        .ControlInput = app_control,
        .Value_1 = app_Read_Function_1,
        .Value_2 = app_Read_Function_2,
        .Config    = app_configure, 
};




void main (void)
{


    int err;


    printk("Starting Bluetooth Controller Prototype \n");

    err = bt_enable(NULL);

    if(err)
    {


        printk("BT init failed\n");
    
    }

    bt_conn_cb_register(&connection_callbacks);


    err = SensorControlInit(&control_callbacks);

    if(err)
    {


        printk("control callback failed initialisation\n");
    
    }

    err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

    if (err)
    {


        printk("advertisement failed\n");
    
    }


    for (;;)
    {   //We are idle from here. Awaiting some action from the BT_RX thread which will eventually
        //drip into the callback functions here. 
        k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
    }    
}
