// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2022 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END

/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include "app.h"
#include "definitions.h"
#include "timers.h"
#include "thread_demo.h"

#include "timers.h"

#include "rgb_led/rgb_led.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

#define LED_BLINK_TIME_MS               (150)

#define LED_TIME_MS                     (100)

#define GREEN_H  85  // 240deg
#define GREEN_S  255  // 100%
#define GREEN_V  255  //100%

#define APP_TEMP_TIMER_INTERVAL_MS     5000

float temp_demo_value = 25.0;
devMsgType_t demoMsg;
devTypeRGBLight_t rgbLED = {1, GREEN_H, GREEN_S, GREEN_V};

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;
extern otInstance *instance;
otIp6Address gatewayAddr;
otUdpSocket aSocket;

/* The timer created for LED that blinks when it receives the data from the Leader */
static TimerHandle_t Data_sent_LED_Timer_Handle = NULL;
/* The timer created for LED that blinks when it sends data to the Leader*/
static TimerHandle_t Data_receive_LED_Timer_Handle = NULL;

static TimerHandle_t LED_Timer_Handle = NULL;

uint8_t autoHue = 0;

devDetails_t threadDevice;
volatile uint16_t temperature_value;

static TimerHandle_t tempTimerHandle = NULL;
void tempTmrCb(TimerHandle_t pxTimer);
// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************
static void Data_sent_LED_Timer_Callback(TimerHandle_t xTimer)
{
//    RGB_LED_GREEN_Off();    
    /* Keep compiler happy. */
     (void)xTimer;    
}
static void Data_receive_LED_Timer_Callback(TimerHandle_t xTimer)
{
    USER_LED_On();   //off
    /* Keep compiler happy. */
     (void)xTimer;    
}

static void LED_Timer_Callback(TimerHandle_t xTimer)
{
    RGB_LED_SetLedColorHSV(autoHue,255,255);
    autoHue += 2;
   
    /* Keep compiler happy. */
     (void)xTimer;    
}


/* TODO:  Add any necessary local functions.
*/


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

//void tempTmrCb(TimerHandle_t pxTimer)
//{
//    //temperature_value = temphum13_get_temperature();
//}

void printIpv6Address(void)
{
//    APP_Msg_T    appMsg;

    const otNetifAddress *unicastAddrs = otIp6GetUnicastAddresses(instance);
    app_printf("Unicast Address :\r\n");
    
//    char string[OT_IP6_ADDRESS_STRING_SIZE];
//    otIp6AddressToString(&(unicastAddrs->mAddress), string, OT_IP6_ADDRESS_STRING_SIZE);
//    app_printf("Unicast Address :\r\n%s\r\n", string);

    for (const otNetifAddress *addr = unicastAddrs; addr; addr = addr->mNext)
    {
        char string[OT_IP6_ADDRESS_STRING_SIZE];
        otIp6AddressToString(&(addr->mAddress), string, OT_IP6_ADDRESS_STRING_SIZE);
        app_printf("%s\r\n", string);
    }
//    appMsg.msgId = ;
//    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
}

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;


    appData.appQueue = xQueueCreate( 64, sizeof(APP_Msg_T) );
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
    
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{
    APP_Msg_T   appMsg;
    APP_Msg_T   *p_appMsg;
    p_appMsg = &appMsg;
    
    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = true;
            //appData.appQueue = xQueueCreate( 10, sizeof(APP_Msg_T) );

            threadAppinit();
            app_printf("App_Log: Thread Network is getting initialized\n");
            
            threadDevice.devType = DEVICE_TYPE_LIGHT;
            threadDevice.devNameSize = sizeof(DEMO_DEVICE_NAME);
            memcpy(&threadDevice.devName, DEMO_DEVICE_NAME, sizeof(DEMO_DEVICE_NAME));
            
            RGB_LED_SetLedColorHSV(rgbLED.hue, rgbLED.saturation, rgbLED.level);

            //tempTimerHandle = xTimerCreate("temp app tmr", (APP_TEMP_TIMER_INTERVAL_MS / portTICK_PERIOD_MS), true, ( void * ) 0, tempTmrCb);
            //xTimerStart(tempTimerHandle, 0);
            if(tempTimerHandle == NULL)
            {
                app_printf("App_Err: App Timer creation failed\n");
            }
                
            if (appInitialized)
            {

                appData.state = APP_STATE_SERVICE_TASKS;
            }
            break;
        }
        case APP_STATE_SERVICE_TASKS:
        {
            if (OSAL_QUEUE_Receive(&appData.appQueue, &appMsg, OSAL_WAIT_FOREVER))
            {
                if(p_appMsg->msgId == APP_MSG_OT_NW_CONFIG_EVT)
                {
                    threadConfigNwParameters();
                }
                else if(p_appMsg->msgId == APP_MSG_OT_NWK_START_EVT)
                {
                    threadNwStart();
                }
                else if(p_appMsg->msgId == APP_MSG_OT_PRINT_IP_EVT)
                {
                    printIpv6Address();
                }
                else if(p_appMsg->msgId == APP_MSG_OT_STATE_HANDLE_EVT)
                {
                    threadHandleStateChange();
                }
                else if(p_appMsg->msgId == APP_MSG_OT_RECV_CB)
                {
                    otMessageInfo *aMessageInfo;
                    otMessage *aMessage;
                    
                    uint8_t aMessageInfoLen = p_appMsg->msgData[0];
                    uint16_t aMessageLen = 0;
                    
                    aMessageLen = (uint16_t)p_appMsg->msgData[aMessageInfoLen + 2];
                    
                    aMessageInfo = (otMessageInfo *)&p_appMsg->msgData[MESSAGE_INFO_INDEX];
                    aMessage = (otMessage *)&p_appMsg->msgData[aMessageInfoLen + 2 + 1];
                    
                    threadReceiveData(aMessageInfo, aMessageLen, (uint8_t *)aMessage);
                }
                else if(p_appMsg->msgId == APP_MSG_OT_SEND_ADDR_TMR_EVT)
                {
                    threadSendPeriodicMsg();
                }
                
            }
            break;
        }

        /* TODO: implement your application state machine.*/


        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}


void threadReceiveData(const otMessageInfo *aMessageInfo, uint16_t length, uint8_t *msgPayload)
{
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    otIp6AddressToString(&(aMessageInfo->mPeerAddr), string, OT_IP6_ADDRESS_STRING_SIZE);
     
    
    USER_LED_Off();
    if( xTimerIsTimerActive( Data_receive_LED_Timer_Handle ) != pdFALSE )
    {
        /* xTimer is active, do something. */
        (void)xTimerStop( Data_receive_LED_Timer_Handle, pdMS_TO_TICKS(0) );
    }
    (void)xTimerStart(Data_receive_LED_Timer_Handle,pdMS_TO_TICKS(0));
    
    devMsgType_t *rxMsg;
    rxMsg = (devMsgType_t *)msgPayload;
    
//    app_printf("App_Log: UDP Received from [%s] len:[%d] type:[%d]\r\n", string, length, rxMsg->msgType);     
    
    
    if(MSG_TYPE_GATEWAY_DISCOVER_REQ == rxMsg->msgType)
    {
        memcpy(&gatewayAddr, rxMsg->msg, OT_IP6_ADDRESS_SIZE);
        demoMsg.msgType = MSG_TYPE_GATEWAY_DISCOVER_RESP;
        memcpy(&demoMsg.msg, &threadDevice, sizeof(devDetails_t));
        threadUdpSend(&gatewayAddr, sizeof(devMsgType_t), (uint8_t *)&demoMsg);
//        app_printf("App Log: DiscReq\r\n");
    }
    else if(MSG_TYPE_LIGHT_SET == rxMsg->msgType)
    {
        app_printf("Rx - %s\n", rxMsg->msg);
        devTypeRGBLight_t *setRGBValue = (devTypeRGBLight_t *)rxMsg->msg;
        rgbLED.onOff = setRGBValue->onOff;
        rgbLED.hue = setRGBValue->hue;
        rgbLED.saturation = setRGBValue->saturation;
        rgbLED.level = setRGBValue->level;
        if( xTimerIsTimerActive(LED_Timer_Handle ) != pdFALSE )
        {
            /* xTimer is active, do something. */
            (void)xTimerStop( LED_Timer_Handle, pdMS_TO_TICKS(0) );
        }
        if(rgbLED.onOff)
        {
            RGB_LED_SetLedColorHSV(rgbLED.hue, rgbLED.saturation, rgbLED.level);
        }
        else
        {
            if(255 == rgbLED.level && 255 == rgbLED.hue && 255 == rgbLED.saturation)
            {
                RGB_LED_SetLedColorHSV(0,255,255);
                (void)xTimerStart(LED_Timer_Handle,pdMS_TO_TICKS(0));
            }
            else
            {
                RGB_LED_SetLedColorHSV(0,0,0);
            }
        }

        demoMsg.msgType = MSG_TYPE_LIGHT_REPORT;

        memcpy(&demoMsg.msg, &rgbLED, sizeof(devTypeRGBLight_t));
        threadUdpSend(&gatewayAddr, sizeof(devMsgType_t), (uint8_t *)&demoMsg);
    }
    else if(MSG_TYPE_LIGHT_GET == rxMsg->msgType)
    {
        demoMsg.msgType = MSG_TYPE_LIGHT_REPORT;

        memcpy(&demoMsg.msg, &rgbLED, sizeof(devTypeRGBLight_t));
        threadUdpSend(&gatewayAddr, sizeof(devMsgType_t), (uint8_t *)&demoMsg);
    }
}

void otUdpReceiveCb(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    APP_Msg_T appMsg_otUdpReceiveCb;
    memset(&appMsg_otUdpReceiveCb, 0, sizeof(APP_Msg_T));
    appMsg_otUdpReceiveCb.msgId = APP_MSG_OT_RECV_CB;
    
    uint16_t aMessageLen = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    uint8_t aMessageInfoLen = sizeof(otMessageInfo);
    
    appMsg_otUdpReceiveCb.msgData[0] = aMessageInfoLen;
    memcpy(&appMsg_otUdpReceiveCb.msgData[MESSAGE_INFO_INDEX], aMessageInfo, sizeof(otMessageInfo));

    appMsg_otUdpReceiveCb.msgData[aMessageInfoLen + MESSAGE_INFO_INDEX + 1] = (uint8_t)aMessageLen;

    otMessageRead(aMessage,otMessageGetOffset(aMessage), &appMsg_otUdpReceiveCb.msgData[aMessageInfoLen + MESSAGE_INFO_INDEX + 2], aMessageLen);
    
//    app_printf("App_Log: Data Received\r\n");
    OSAL_QUEUE_SendISR(&appData.appQueue, &appMsg_otUdpReceiveCb);
//    OSAL_QUEUE_Send(&appData.appQueue, &appMsg_otUdpReceiveCb, 0);
}

//void otUdpReceiveCb(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
//{
//    APP_Msg_T appMsg_otUdpReceiveCb;
//    
//    uint16_t len = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
//    uint8_t output_buffer[len+1];
//    
//    otMessageRead(aMessage,otMessageGetOffset(aMessage),output_buffer,len);
//    output_buffer[len] = '\0';
//    
//    
//    appMsg_otUdpReceiveCb.msgId = APP_MSG_OT_RECV_CB;
//    
//    otUdpReceiveData_t *otUdpReceiveData;
//    otUdpReceiveData = (otUdpReceiveData_t *)&appMsg_otUdpReceiveCb;
//        
//    memcpy(otUdpReceiveData->messageInfo, aMessageInfo, sizeof(otMessageInfo));
//    otUdpReceiveData->length = (uint8_t)len;
//    memcpy(otUdpReceiveData->msgPayload, &output_buffer, len);
//
//    OSAL_QUEUE_Send(&appData.appQueue, &appMsg_otUdpReceiveCb, 0);
//    
//}

void threadUdpOpen()
{
   otError err;
   app_printf("App_log: UDP Open\n");
   err = otUdpOpen(instance, &aSocket, otUdpReceiveCb, NULL);
   if (err != OT_ERROR_NONE)
   {
      app_printf("App_Err: UDP Open failed\n");
       //print error code
       assert(err);
   }
    /* The timer created for LED that blinks when it receives the data from the Leader */
    Data_sent_LED_Timer_Handle = xTimerCreate("Milli_Timer",pdMS_TO_TICKS(LED_BLINK_TIME_MS),pdFALSE, ( void * ) 0, Data_sent_LED_Timer_Callback);
    /* The timer created for LED that blinks when it sends data to the Leader*/
    Data_receive_LED_Timer_Handle = xTimerCreate("Milli_Timer",pdMS_TO_TICKS(LED_BLINK_TIME_MS),pdFALSE, ( void * ) 0, Data_receive_LED_Timer_Callback);
    
    LED_Timer_Handle = xTimerCreate("Milli_Timer",pdMS_TO_TICKS(LED_TIME_MS),true, ( void * ) 0, LED_Timer_Callback);
}

void threadUdpSend(otIp6Address *mPeerAddr, uint8_t msgLen, uint8_t* msg)
{
    otError err = OT_ERROR_NONE;
    otMessageInfo msgInfo;
//    const otIp6Address *mPeerAddr;
    const otIp6Address *mSockAddr;
    memset(&msgInfo,0,sizeof(msgInfo));
//    otIp6AddressFromString("ff03::1",&msgInfo.mPeerAddr);
    mSockAddr = otThreadGetMeshLocalEid(instance);
//    mPeerAddr = otThreadGetRealmLocalAllThreadNodesMulticastAddress(instance);
    memcpy(&msgInfo.mSockAddr, mSockAddr, OT_IP6_ADDRESS_SIZE);
    memcpy(&msgInfo.mPeerAddr, mPeerAddr, OT_IP6_ADDRESS_SIZE);
    
    msgInfo.mPeerPort = UDP_PORT_NO;
    
    do {
        otMessage *udp_msg = otUdpNewMessage(instance,NULL);
        err = otMessageAppend(udp_msg,msg,msgLen);
        if(err != OT_ERROR_NONE)
        {
            app_printf("App_Err: UDP Message Add fail\n");
            break;
        }
        
        err = otUdpSend(instance,&aSocket,udp_msg,&msgInfo);
        if(err != OT_ERROR_NONE)
        {
            app_printf("App_Err: UDP Send fail\n");
            break;
        }
        app_printf("App_Log: UDP Sent data: %d\r\n",err);
//        RGB_LED_GREEN_On();
        if( xTimerIsTimerActive( Data_sent_LED_Timer_Handle ) != pdFALSE )
        {
            /* xTimer is active, do something. */
            (void)xTimerStop( Data_sent_LED_Timer_Handle, pdMS_TO_TICKS(0) );
        }
        (void)xTimerStart(Data_sent_LED_Timer_Handle,pdMS_TO_TICKS(0));

    }while(false);
    
}

//void threadUdpSendAddress(otIp6Address mPeerAddr)
//{
//    otError err = OT_ERROR_NONE;
//    otMessageInfo msgInfo;
//    memset(&msgInfo,0,sizeof(msgInfo));
//    memcpy(&msgInfo.mPeerAddr, &mPeerAddr, OT_IP6_ADDRESS_SIZE);
//    msgInfo.mPeerPort = UDP_PORT_NO;
//    
//    do {
//        otMessage *udp_msg = otUdpNewMessage(instance,NULL);
//        err = otMessageAppend(udp_msg,msg,(uint16_t)strlen(msg));
//        if(err != OT_ERROR_NONE)
//        {
//            app_printf("App_Err: UDP Message Add fail\n");
//            break;
//        }
//        
//        err = otUdpSend(instance,&aSocket,udp_msg,&msgInfo);
//        if(err != OT_ERROR_NONE)
//        {
//            app_printf("App_Err: UDP Send fail\n");
//            break;
//        }
//        app_printf("App_Log: UDP Sent data: %s\n",msg);
//        RGB_LED_GREEN_On();
//        if( xTimerIsTimerActive( Data_sent_LED_Timer_Handle ) != pdFALSE )
//        {
//            /* xTimer is active, do something. */
//            (void)xTimerStop( Data_sent_LED_Timer_Handle, pdMS_TO_TICKS(0) );
//        }
//        (void)xTimerStart(Data_sent_LED_Timer_Handle,pdMS_TO_TICKS(0));
//        
//    }while(false);
//}

void threadUdpBind()
{
   otError err;
   otSockAddr addr;
   memset(&addr,0,sizeof(otSockAddr));
   addr.mPort = UDP_PORT_NO;
   do
   {
        err = otUdpBind(instance, &aSocket, &addr, OT_NETIF_THREAD);
        if (err != OT_ERROR_NONE) {
            app_printf("App_Err: UDP Bind fail Err:%d\n",err);
            break;
        }
        app_printf("App_Log: UDP Listening on port %d\n",UDP_PORT_NO);
   }while(false);
}

//void threadUdpReceiveCb(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
//{
//    uint16_t len = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
//    uint8_t output_buffer[len+1];
//    char string[OT_IP6_ADDRESS_STRING_SIZE];
//    otIp6AddressToString(&(aMessageInfo->mPeerAddr), string, OT_IP6_ADDRESS_STRING_SIZE);
//    
//    otMessageRead(aMessage,otMessageGetOffset(aMessage),output_buffer,len);
//    output_buffer[len] = '\0';
//    app_printf("App_Log: UDP Received from %s data: %s\n", string, output_buffer);
//    threadReceiveData((otIp6Address *)&(aMessageInfo->mPeerAddr), len, output_buffer);
//    USER_LED_Off();
//    if( xTimerIsTimerActive( Data_receive_LED_Timer_Handle ) != pdFALSE )
//    {
//        /* xTimer is active, do something. */
//        (void)xTimerStop( Data_receive_LED_Timer_Handle, pdMS_TO_TICKS(0) );
//    }
//    (void)xTimerStart(Data_receive_LED_Timer_Handle,pdMS_TO_TICKS(0));
//    
//#if (DEVICE_AS_LEADER==0)
////    threadUdpSendAddress(aMessageInfo->mPeerAddr);
//#endif
//}


/*******************************************************************************
 End of File
 */
