//This is the header file for the BLE control register and sensor services
/*

Function 1 = Temperature Value (Example, can be anything)
Function 2 = Moisture Value    (Example, can be anything)


a7af10f4-0734-45d7-bd82-f0c2d4f152e7 Service

08b309cc-abe3-423a-a7b3-215f4269c379 Status Characteristic

690da2f0-50db-46ba-b597-a3067f6b4978 Temperature Value Attribute /Function 1

a57ca969-1547-4b3b-af04-5cb28929ea35 Moisture Value Attribute / Function 2

d3790152-5bd7-48d3-8fda-9d4de08657a0 Control Register Attribute

9e260304-1eb0-4e57-9663-50f25a6e045a Config Register Attribute 

fdf80904-6192-4dff-a04b-8b84e43b18a4 Future use

7b05096c-e7a8-4482-b45e-7d039d4a8a90 Future use

cddba84a-0ebd-43b1-acb9-dcd65b6e1d99 Future use

cbb57e3f-748f-4640-8f58-d92ec12165c1 Future use

Above are the randomly generated 128-bit UUID's used 


Based on how Nordic Semis applications work with their samples, I believe the above adresses, which are fixed,
are translated into proper names on the client side, that is to say on the app-side in our case.
That also seems like the easiest way to do it. It is possible to have the value in an attribute be a string,
but the name of it may still be presented as a jumble of numbers and letters if not translated somewhere.

The addresses above are randomly generated, which is the way to do it unless the organisation wants to pay to
be included in the official range.
*/
#include <zephyr/types.h>


enum STATUS_REGISTER_VALUES 
{
    Idle = 0,

    Measuring = 1,
};

enum CONTROL_REGISTER_VALUES
{
    MeasureTemperature = 1, // 0001

    MeasureMoisture    = 2, // 0010
};

enum CONFIG_REGISTER_VALUES
{
    ToggleTemperature = 1, // 0001    

    ToggleMoisture    = 2, // 0010
};

/*
    The structure below is BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(Your 128 bit value as five hex-fields)) for every attribute.
    The declared ID's are later used in the other code files.

    This is a bit complicated, but essentially, the service, a kind of headline for the other attributes, or the box
    which contains them, is itself an attribute, but with a certain field (not visible in this code) which makes it 
    a service. Thus it is created in the same way as all the concrete characteristics. 
*/


#define BT_UUID_SENSORCONTROL_SERVICE_val       BT_UUID_128_ENCODE(0xa7af10f4, 0x0734, 0x45d7, 0xbd82, 0xf0c2d4f152e7)
#define BT_UUID_STATUS_val                      BT_UUID_128_ENCODE(0x08b309cc, 0xabe3, 0x423a, 0xa7b3, 0x215f4269c379)
#define BT_UUID_TEMP_VALUE_val                  BT_UUID_128_ENCODE(0x690da2f0, 0x50db, 0x46ba, 0xb597, 0xa3067f6b4978) 
#define BT_UUID_MOIST_VALUE_val                 BT_UUID_128_ENCODE(0xa57ca969, 0x1547, 0x4b3b, 0xaf04, 0x5cb28929ea35)
#define BT_UUID_CONTROL_val                     BT_UUID_128_ENCODE(0xd3790152, 0x5bd7, 0x48d3, 0x8fda, 0x9d4de08657a0)
#define BT_UUID_CONFIG_val                      BT_UUID_128_ENCODE(0x9e260304, 0x1eb0, 0x4e57, 0x9663, 0x50f25a6e045a)



#define BT_UUID_SENSORCONTROL_SERVICE       BT_UUID_DECLARE_128(BT_UUID_SENSORCONTROL_SERVICE_val)
#define BT_UUID_STATUS                      BT_UUID_DECLARE_128(BT_UUID_STATUS_val)
#define BT_UUID_TEMP_VALUE                  BT_UUID_DECLARE_128(BT_UUID_TEMP_VALUE_val)
#define BT_UUID_MOIST_VALUE                 BT_UUID_DECLARE_128(BT_UUID_MOIST_VALUE_val)    
#define BT_UUID_CONTROL                     BT_UUID_DECLARE_128(BT_UUID_CONTROL_val)
#define BT_UUID_CONFIG                      BT_UUID_DECLARE_128(BT_UUID_CONFIG_val)



/*
    Below are the callback functions. While the bluetooth callback structure can be complicated to follow
    it is not as bad as it seems.

    Example:  
    1: Radio Waves reach the Bluetooth controller. This ends up creating a work item in the Bluetooth RX thread
    2: Eventually, this work item is handled, which places us in the SensorControl.c file, let us say the ShowStatus function
    3: static ssize_t Give_Sensor_Status will then execute. Eventually, it will reach the following line:
    4: Status_Register = BT_control_cb.Status();. This is where Status register, an unsigned long, will be given a value
    5: But from what, what is BT_control_cb.Status()? A function pointer, declared in SensorControl.c => static struct control_CB    BT_control_cb;
    (The general callback struct is defined below in this file)
    6: But that is just a declaration, what is in the struct? That is determined by the init function defined in SensorControl.c
    
    int SensorControlInit (struct control_CB *callbacks)
    {
        if (callbacks)
        {
            BT_control_cb.Status = callbacks->Status;
            ......
    7: In other words, the SensorControlInit function is called with a control_CB struct, and
       and this makes the function pointers in sensorcontrol.c point to real functions. 
       So where do we make BT_control_cb.Status point to a concrete function?
    8: This finally brings us to main. in main.c, we call SensorControlInit with a struct filled with functions we have defined
       in main. Some functions queue work items, but this function simply returns a static variable.
    9: The point is that it can be anything, this structure essentially splits everything in two, 
       so that if one does not quite understand Bluetooth, one can still modify what happens due to a specific 
       bluetooth request or action in main.c, while similarly one can modify the Bluetooth connection layer 
       in the sensorcontrol files without changing any functonality on the "physical" side,
       and at most one can ruin only half the system at one time. 
*/   

typedef void            (*RegisterControl_type) (uint8_t); 
typedef uint8_t         (*ShowStatus_type)      (void);
typedef int             (*Func_1_Read_type)        (void);
typedef uint8_t         (*Func_2_Read_type)       (void);
typedef void            (*ConfigRegister_type)  (uint8_t);
       

struct control_CB 
{
    ShowStatus_type         Status;           
    RegisterControl_type    ControlInput;     
    Func_1_Read_type        Value_1;
    Func_2_Read_type        Value_2;
    ConfigRegister_type     Config;
};






int SensorControlInit(struct control_CB *callbacks);

